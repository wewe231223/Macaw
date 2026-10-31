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
#include "Asset/UStaticMesh.h"
#include "FObjectConstants.h"
#include "FScenePrimitive.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>

namespace Renderer {

    /**
     * @brief FScene의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Renderer::FScene::FScene(ID3D11Device* Device, ID3D11DeviceContext* Context)
		: mDevice{Device},
		  mContext{Context} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Renderer::FScene::~FScene() = default;

    void Renderer::FScene::AddHeightFog(const Engine::FHeightFogSceneProxy& Proxy) {
        const auto Existing{std::find_if(mHeightFogs.begin(), mHeightFogs.end(), [&Proxy](const Engine::FHeightFogSceneProxy& Fog) {
            return Fog.mId == Proxy.mId;
        })};

        if (Existing != mHeightFogs.end()) {
            *Existing = Proxy;
        } else {
            mHeightFogs.push_back(Proxy);
        }

        ++mRevision;
    }

    void Renderer::FScene::RemoveHeightFog(const Core::FGuid& Id) {
        if (std::erase_if(mHeightFogs, [&Id](const Engine::FHeightFogSceneProxy& Fog) {
            return Fog.mId == Id;
        }) > 0) {
            ++mRevision;
        }
    }

    const Core::TArray<Engine::FHeightFogSceneProxy>& Renderer::FScene::GetHeightFogs() const {
        return mHeightFogs;
    }

    void Renderer::FScene::AddPrimitive(const Engine::FPrimitiveSceneProxy& Proxy) {
        if (Proxy.mMesh != nullptr && !Proxy.mMesh->CreateResources(mDevice)) {
            Core::ErrorHandler::Report("Mesh", "Cannot create project mesh resources", Core::ErrorHandler::EErrorLevel::Error);
            return;
        }

        const auto Position{mIndices.find(Proxy.mId)};

        bool CasterChanged{IsShadowCaster(Proxy)};

        if (Position != mIndices.end()) {
            Renderer::FScenePrimitive& Primitive{mPrimitives[Position->second]};
            CasterChanged = CasterChanged || IsShadowCaster(Primitive.mProxy);
            Primitive.mProxy = Proxy;

            Upload(Primitive);
        } else {
            Renderer::FScenePrimitive Primitive{Proxy};

            Upload(Primitive);

            mIndices.emplace(Proxy.mId, mPrimitives.size());
            mPrimitives.push_back(std::move(Primitive));
        }

        if (CasterChanged) {
            mCasterRevision = Core::FRevision::Next();
        }

        ++mRevision;
    }

    /**
     * @brief 지정한 프리미티브의 월드 행렬과 경계 상자를 갱신합니다.
     */
    void Renderer::FScene::UpdatePrimitiveTransform(const Core::FGuid& Id, const Core::FMatrix& World, const Core::FBox& Bounds) {
        const auto Position{mIndices.find(Id)};

        if (Position == mIndices.end()) {
            return;
        }

        Renderer::FScenePrimitive& Primitive{mPrimitives[Position->second]};

        if (Primitive.mProxy.mWorld == World) {
            return;
        }

        Primitive.mProxy.mWorld = World;
        Primitive.mProxy.mBounds = Bounds;

        Upload(Primitive);

        if (IsShadowCaster(Primitive.mProxy)) {
            mCasterRevision = Core::FRevision::Next();
        }

        ++mRevision;
    }

    /**
     * @brief 식별자에 해당하는 프리미티브와 관련 GPU 리소스를 제거합니다.
     */
    void Renderer::FScene::RemovePrimitive(const Core::FGuid& Id) {
        const auto Position{mIndices.find(Id)};

        if (Position == mIndices.end()) {
            return;
        }

        const std::size_t Index{Position->second};

        if (IsShadowCaster(mPrimitives[Index].mProxy)) {
            mCasterRevision = Core::FRevision::Next();
        }

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
    std::size_t Renderer::FScene::GetPrimitiveCount() const {
        return mPrimitives.size();
    }

    /**
     * @brief 프리미티브 추가, 제거 또는 변환 갱신 시 증가하는 장면 개정 번호를 반환합니다.
     */
    Core::Uint64 Renderer::FScene::GetRevision() const {
        return mRevision;
    }

    /**
     * @brief 장면이 소유한 렌더 프리미티브를 반환합니다.
     */
    const Core::TArray<Renderer::FScenePrimitive>& Renderer::FScene::GetPrimitives() const {
        return mPrimitives;
    }

    /**
     * @brief 누적 변환 업로드 횟수를 반환하고 카운터를 초기화합니다.
     */
    Core::FRevision Renderer::FScene::GetIdentity() const {
        return mIdentity;
    }

    Core::FRevision Renderer::FScene::GetCasterRevision() const {
        Core::FRevision Revision{mCasterRevision};

        for (const Renderer::FScenePrimitive& Primitive : mPrimitives) {
            if (IsShadowCaster(Primitive.mProxy) && Primitive.mProxy.mMesh != nullptr && Primitive.mProxy.mMesh->GetRevision().GetValue() > Revision.GetValue()) {
                Revision = Primitive.mProxy.mMesh->GetRevision();
            }
        }

        return Revision;
    }

    bool Renderer::FScene::IsShadowCaster(const Engine::FPrimitiveSceneProxy& Proxy) {
        return Proxy.mCastShadow && !Proxy.mSprite.has_value() && !Proxy.mText.has_value();
    }

    Core::Uint32 Renderer::FScene::TakeTransformUpdates() const {
        const Core::Uint32 Count{mTransformUpdates};

        mTransformUpdates = 0;

        return Count;
    }

    /**
     * @brief 프리미티브의 객체 상수 버퍼를 현재 프록시 값으로 갱신합니다.
     */
    void Renderer::FScene::Upload(Renderer::FScenePrimitive& Primitive) {
        if (Primitive.mProxy.mText.has_value() || Primitive.mProxy.mSprite.has_value()) {
            Primitive.mObjectBuffer.Reset();
            return;
        }

        Renderer::FObjectConstants& Constants{Primitive.mObjectConstants};
        Constants.mWorld = Primitive.mProxy.mWorld;
        Constants.mShadowFlags.x = Primitive.mProxy.mReceiveShadow ? 1.0f : 0.0f;
        Constants.mUVTransform = {Primitive.mProxy.mUVScale.x, Primitive.mProxy.mUVScale.y, Primitive.mProxy.mUVOffset.x, Primitive.mProxy.mUVOffset.y};

        if (!Constants.mWorld.TryInverse(Constants.mWorldInverse)) {
            Core::ErrorHandler::Fatal("Renderer", "Singular render transform");
        }

        if (Primitive.mObjectBuffer == nullptr) {
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = sizeof(Renderer::FObjectConstants);
            Description.Usage = D3D11_USAGE_DEFAULT;
            Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

            D3D11_SUBRESOURCE_DATA Data{};
            Data.pSysMem = &Constants;

            if (FAILED(mDevice->CreateBuffer(&Description, &Data, Primitive.mObjectBuffer.GetAddressOf()))) {
                Core::ErrorHandler::Fatal("Renderer", "Object buffer allocation failed");
            }
        } else {
            mContext->UpdateSubresource(Primitive.mObjectBuffer.Get(), 0, nullptr, &Constants, 0, 0);
        }

        ++mTransformUpdates;
    }

    void Renderer::FScene::AddLight(const Engine::FLightSceneProxy& Proxy) {
        const auto Position{mLightIndices.find(Proxy.mId)};

        if (Position == mLightIndices.end()) {
            mLightIndices.emplace(Proxy.mId, mLights.size());
            mLights.push_back(Proxy);
        } else {
            mLights[Position->second] = Proxy;
        }

        ++mRevision;
    }

    void Renderer::FScene::RemoveLight(const Core::FGuid& Id) {
        const auto Position{mLightIndices.find(Id)};

        if (Position == mLightIndices.end()) {
            return;
        }

        const std::size_t Index{Position->second};

        if (Index + 1 != mLights.size()) {
            mLights[Index] = mLights.back();
            mLightIndices[mLights[Index].mId] = Index;
        }

        mLights.pop_back();
        mLightIndices.erase(Position);
        ++mRevision;
    }

    const Core::TArray<Engine::FLightSceneProxy>& Renderer::FScene::GetLights() const {
        return mLights;
    }

}
