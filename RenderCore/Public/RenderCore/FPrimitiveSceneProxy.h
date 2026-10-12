#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/FObjectHandle.h"
#include "RenderCore/FUVTransform.h"

#include <span>

class FStaticPrimitiveDrawInterface;
class FDynamicPrimitiveDrawInterface;

struct FPrimitiveTransform {
    FMatrix mWorld{};
    DirectX::BoundingSphere mWorldSphereBounds{};
    DirectX::BoundingOrientedBox mWorldOBB{};
    DirectX::BoundingBox mWorldAABB{};
};

struct FMeshSceneData {
    FAssetHandle mMeshHandle{};
    FAssetHandle mMaterialHandle{};
    FAssetHandle mPipelineHandle{};
};

class FPrimitiveSceneProxy {
public:
    FPrimitiveSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform);
    virtual ~FPrimitiveSceneProxy();
    FPrimitiveSceneProxy(const FPrimitiveSceneProxy&) = delete;
    FPrimitiveSceneProxy& operator=(const FPrimitiveSceneProxy&) = delete;
    FPrimitiveSceneProxy(FPrimitiveSceneProxy&&) = delete;
    FPrimitiveSceneProxy& operator=(FPrimitiveSceneProxy&&) = delete;

public:
    FObjectHandle GetComponentHandle() const;
    FObjectHandle GetOwnerHandle() const;
    const FPrimitiveTransform& GetTransform() const;
    virtual const FMeshSceneData* GetMeshData() const;
    virtual void DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const;
    virtual void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const;
    void SetTransform(const FPrimitiveTransform& Transform);
    virtual void SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax);
    void SetUVTransforms(std::span<const FPrimitiveUVTransform> Transforms);
    FUVTransform GetUVTransform(FName MaterialGroup = {}) const;

private:
    FObjectHandle mComponentHandle{};
    FObjectHandle mOwnerHandle{};
    FPrimitiveTransform mTransform{};
    TArray<FPrimitiveUVTransform> mUVTransforms{};
};
