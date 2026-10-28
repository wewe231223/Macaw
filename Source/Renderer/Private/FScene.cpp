/**
 * @file FScene.cpp
 * @brief 프리미티브 프록시와 객체 상수 버퍼를 소유하는 Direct3D 11 렌더 장면입니다.
 * @ingroup Renderer
 */

#include "FScene.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/UStaticMesh.h"
#include "FObjectConstants.h"
#include "FScenePrimitive.h"
#include "Core/Base/ErrorHandler.h"

/**
 * @brief FScene의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FScene::FScene(ID3D11Device* Device, ID3D11DeviceContext* Context)
	: mDevice{Device},
	  mContext{Context} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FScene::~FScene() = default;

/**
 * @brief 프록시의 식별자로 렌더 장면에 프리미티브를 추가하거나 교체합니다.
 */
void FScene::AddPrimitive(const FPrimitiveSceneProxy& Proxy) {
    if (Proxy.mMesh != nullptr && !Proxy.mMesh->CreateResources(mDevice)) {
        ErrorHandler::Report("Mesh", "Cannot create project mesh resources", ErrorHandler::EErrorLevel::Error);
        return;
    }
    const auto Position{mIndices.find(Proxy.mId)};

    if (Position != mIndices.end()) {
        FScenePrimitive& Primitive{mPrimitives[Position->second]};
        Primitive.mProxy = Proxy;

        Upload(Primitive);
    } else {
        FScenePrimitive Primitive{Proxy};

        Upload(Primitive);

        mIndices.emplace(Proxy.mId, mPrimitives.size());
        mPrimitives.push_back(std::move(Primitive));
    }

    ++mRevision;
}

/**
 * @brief 지정한 프리미티브의 월드 행렬과 경계 상자를 갱신합니다.
 */
void FScene::UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) {
    const auto Position{mIndices.find(Id)};

    if (Position == mIndices.end()) {
        return;
    }

    FScenePrimitive& Primitive{mPrimitives[Position->second]};

    if (Primitive.mProxy.mWorld == World) {
        return;
    }

    Primitive.mProxy.mWorld = World;
    Primitive.mProxy.mBounds = Bounds;

    Upload(Primitive);

    ++mRevision;
}

/**
 * @brief 식별자에 해당하는 프리미티브와 관련 GPU 리소스를 제거합니다.
 */
void FScene::RemovePrimitive(const FGuid& Id) {
    const auto Position{mIndices.find(Id)};

    if (Position == mIndices.end()) {
        return;
    }

    const std::size_t Index{Position->second};

    if (Index + 1 != mPrimitives.size()) {
        mPrimitives[Index] = std::move(mPrimitives.back());
        mIndices[mPrimitives[Index].mProxy.mId] = Index;
    }

    mPrimitives.pop_back();
    mIndices.erase(Position);

    ++mRevision;
}

/**
 * @brief 현재 장면에 등록된 프리미티브 개수를 반환합니다.
 */
std::size_t FScene::GetPrimitiveCount() const {
    return mPrimitives.size();
}

/**
 * @brief 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호를 반환합니다.
 */
Uint64 FScene::GetRevision() const {
    return mRevision;
}

/**
 * @brief 장면이 소유한 렌더 프리미티브를 반환합니다.
 */
const TArray<FScenePrimitive>& FScene::GetPrimitives() const {
    return mPrimitives;
}

/**
 * @brief 누적 변환 업로드 횟수를 반환하고 카운터를 초기화합니다.
 */
Uint32 FScene::TakeTransformUpdates() const {
    const Uint32 Count{mTransformUpdates};
    mTransformUpdates = 0;

    return Count;
}

/**
 * @brief 프리미티브의 객체 상수 버퍼를 현재 프록시 값으로 갱신합니다.
 */
void FScene::Upload(FScenePrimitive& Primitive) {
    if (Primitive.mProxy.mText.has_value() || Primitive.mProxy.mSprite.has_value()) {
        Primitive.mObjectBuffer.Reset();
        return;
    }
    FObjectConstants Constants{};
    Constants.mWorld = Primitive.mProxy.mWorld;
    Constants.mUVTransform = {Primitive.mProxy.mUVScale.x, Primitive.mProxy.mUVScale.y, Primitive.mProxy.mUVOffset.x, Primitive.mProxy.mUVOffset.y};

    if (!Constants.mWorld.TryInverse(Constants.mWorldInverse)) {
        ErrorHandler::Fatal("Renderer", "Singular render transform");
    }

    if (Primitive.mObjectBuffer == nullptr) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = sizeof(FObjectConstants);
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        D3D11_SUBRESOURCE_DATA Data{};
        Data.pSysMem = &Constants;

        if (FAILED(mDevice->CreateBuffer(&Description, &Data, Primitive.mObjectBuffer.GetAddressOf()))) {
            ErrorHandler::Fatal("Renderer", "Object buffer allocation failed");
        }
    } else {
        mContext->UpdateSubresource(Primitive.mObjectBuffer.Get(), 0, nullptr, &Constants, 0, 0);
    }

    ++mTransformUpdates;
}
