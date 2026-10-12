#pragma once
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/FMeshPickingProxy.h"
#include "Core/Base/FAssetHandle.h"
#include "Core/Asset/FAssetPath.h"
#include "Core/Base/FGuid.h"
#include "Asset/UMesh.h"
#include "World/Component/UMeshComponent.generated.h"

UCLASS(Abstract)
class UMeshComponent : public UPrimitiveComponent {
public:
    GENERATED_TYPES(UMeshComponent)

public:
    UMeshComponent();
    ~UMeshComponent() override = default;

public:
    GENERATED_BODY(UMeshComponent)

    FAssetHandle GetMeshHandle() const;
    virtual void SetMeshHandle(FAssetHandle InHandle);
    void OnRegister() override;

    virtual const UMesh* ResolveMesh() const;
    bool BuildPickingBoxFromMesh();
    bool RaycastMesh(const FRay& Ray, float& OutDistance, float MaxDistance = std::numeric_limits<float>::max()) const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    mutable FMeshPickingProxy mRaycastProxy;
    mutable Uint64 mRaycastTransformRevision = 0;
    mutable bool mRaycastTransformInitialized = false;
    FAssetHandle mMeshHandle{};
    FAssetPath mMeshAssetPath{};
    FGuid mMeshAssetGuid{};
};
