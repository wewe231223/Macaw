#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/FMeshPickingProxy.h"
#include "Core/Base/FAssetHandle.h"
#include "Core/Asset/FAssetPath.h"
#include "Core/Base/FGuid.h"
#include "Asset/UMesh.h"

class FProperty;

class UMeshComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UMeshComponent, UPrimitiveComponent);

public:
    UMeshComponent();
    ~UMeshComponent() override = default;

public:
    ReflectClass(UMeshComponent, EClassFlags::Abstract);

    FAssetHandle GetMeshHandle() const;
    virtual void SetMeshHandle(FAssetHandle InHandle);
    void OnRegister() override;

    virtual const UMesh* ResolveMesh() const;
    bool BuildPickingBoxFromMesh();
    bool RaycastMesh(const FRay& Ray, float& OutDistance, float MaxDistance = std::numeric_limits<float>::max()) const;

protected:
    void Serialize(FArchive& Archive) override;

private:
    static void SerializeMeshHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);

private:
    mutable FMeshPickingProxy mRaycastProxy{};
    mutable Uint64 mRaycastTransformRevision{};
    mutable bool mRaycastTransformInitialized{};
    ReflectAccessor(mMeshHandle, GetMeshHandle, SetMeshHandle, "MeshHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializeMeshHandleProperty, ReflectEditor("Rendering", "Mesh"), ReflectAsset(UMesh));
    FAssetHandle mMeshHandle{};
    FAssetPath mMeshAssetPath{};
    FGuid mMeshAssetGuid{};

    ReflectMembers();
};
