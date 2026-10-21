#include "FScene.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FBox.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/EPrimitiveType.h"
#include "FObjectConstants.h"
#include "FScenePrimitive.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>

FScene::FScene(ID3D11Device& Device, ID3D11DeviceContext& Context)
	: mDevice{&Device},
	  mContext{&Context} {
}

FScene::~FScene() = default;

void FScene::AddPrimitive(const FPrimitiveSceneProxy& Proxy) {
    const auto Position{mIndices.find(Proxy.mId)};

    if (Position != mIndices.end()) {
        FScenePrimitive& Primitive{mPrimitives[Position->second]};
        Primitive.mProxy = Proxy;

        Upload(Primitive);
    } else {
        FScenePrimitive Primitive{};
        Primitive.mProxy = Proxy;

        Upload(Primitive);

        mIndices.emplace(Proxy.mId, mPrimitives.size());
        mPrimitives.push_back(std::move(Primitive));
    }

    ++mRevision;
}

void FScene::UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) {
    const auto Position{mIndices.find(Id)};

    if (Position == mIndices.end()) {
        return;
    }

    FScenePrimitive& Primitive{mPrimitives[Position->second]};

    if (std::ranges::equal(Primitive.mProxy.mWorld.mValues, World.mValues, [](const auto& Left, const auto& Right) {
        return std::ranges::equal(Left, Right);
    })) {
        return;
    }

    Primitive.mProxy.mWorld = World;
    Primitive.mProxy.mBounds = Bounds;

    Upload(Primitive);

    ++mRevision;
}

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

std::size_t FScene::GetPrimitiveCount() const {
    return mPrimitives.size();
}

Uint64 FScene::GetRevision() const {
    return mRevision;
}

const TArray<FScenePrimitive>& FScene::GetPrimitives() const {
    return mPrimitives;
}

Uint32 FScene::TakeTransformUpdates() const {
    const Uint32 Count{mTransformUpdates};
    mTransformUpdates = 0;

    return Count;
}

void FScene::Upload(FScenePrimitive& Primitive) {
    FObjectConstants Constants{};
    Constants.mWorld = Primitive.mProxy.mWorld;

    if (!Constants.mWorld.TryInverse(Constants.mWorldInverse)) {
        ErrorHandler::Fatal("Renderer", "Singular render transform");
    }

    switch (Primitive.mProxy.mPrimitive) {
    case EPrimitiveType::Plane:
        Constants.mColor = {0.28f, 0.39f, 0.43f, 1.0f};
        break;

    case EPrimitiveType::Sphere:
        Constants.mColor = {0.88f, 0.53f, 0.23f, 1.0f};
        break;

    default:
        Constants.mColor = {0.28f, 0.60f, 0.78f, 1.0f};
        break;
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
