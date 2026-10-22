#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FSceneInterface.h"
#include "Core/GuidHash.h"
#include "FScenePrimitive.h"

#include <d3d11.h>
#include <wrl/client.h>

class FScene final : public FSceneInterface {
public:
    ReflectTypes(FScene, FSceneInterface);

public:
    FScene(ID3D11Device& Device, ID3D11DeviceContext& Context);
    ~FScene() override;

public:
    ReflectStruct(FScene);

    void AddPrimitive(const FPrimitiveSceneProxy& Proxy) override;
    void UpdatePrimitiveTransform(const FGuid& Id, const FMatrix& World, const FBox& Bounds) override;
    void RemovePrimitive(const FGuid& Id) override;

    std::size_t GetPrimitiveCount() const override;
    Uint64 GetRevision() const override;

    const TArray<FScenePrimitive>& GetPrimitives() const;
    Uint32 TakeTransformUpdates() const;

private:
    void Upload(FScenePrimitive& Primitive);

private:
    ReflectMembers();

private:
    ID3D11Device* mDevice{};
    ReflectProperty(mDevice, "Device", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    ID3D11DeviceContext* mContext{};
    ReflectProperty(mContext, "Context", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    TArray<FScenePrimitive> mPrimitives{};
    ReflectProperty(mPrimitives, "Primitives", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TMap<FGuid, std::size_t, FGuidHash> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    Uint64 mRevision{};
    ReflectProperty(mRevision, "Revision", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    mutable Uint32 mTransformUpdates{};
    ReflectProperty(mTransformUpdates, "TransformUpdates", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
