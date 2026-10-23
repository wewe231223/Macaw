#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "World/Component/UMeshComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMesh.h"

FAssetHandle UMeshComponent::GetMeshHandle() const {
    return mMeshReference.GetHandle();
}

void UMeshComponent::SetMeshHandle(FAssetHandle InHandle) {
    mMeshReference.Assign(InHandle, FAssetRegistry::TryGet());
    BuildPickingBoxFromMesh();
    OnRenderStateChanged();
}

const UMesh* UMeshComponent::ResolveMesh() const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    return Registry != nullptr ? Registry->ResolveAsset<UMesh>(mMeshReference.GetHandle()) : nullptr;
}

void UMeshComponent::OnRegister() {
    Super::OnRegister();
    mMeshReference.Resolve(FAssetRegistry::TryGet());
    BuildPickingBoxFromMesh();
}

bool UMeshComponent::BuildPickingBoxFromMesh() {
    const UMesh* Mesh{ResolveMesh()};

    if (Mesh == nullptr) {
        return false;
    }

    SetPickingBox(Mesh->GetBoundingBox());

    return true;
}

bool UMeshComponent::RaycastMesh(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const {
    const UMesh* Mesh{ResolveMesh()};
    const Uint64 Revision{GetTransformRevision()};

    if (!mRaycastTransformInitialized || mRaycastTransformRevision != Revision || mRaycastProxy.GetMesh() != Mesh) {
        mRaycastProxy.Update(Mesh, GetComponentTransform());
        mRaycastTransformRevision = Revision;
        mRaycastTransformInitialized = true;
    }

    OutHit = {};

    if (!mRaycastProxy.Raycast(Ray, OutHit, MaxDistance)) {
        return false;
    }

    OutHit.mComponent.Set(const_cast<UMeshComponent*>(this));
    return true;
}

void UMeshComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        BuildPickingBoxFromMesh();
        OnRenderStateChanged();
    }
}

UMeshComponent::UMeshComponent() {
    InitializeClass(StaticClass());
}

void UMeshComponent::SerializeMeshHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UMeshComponent& Object{*static_cast<UMeshComponent*>(Container)};

    SerializeAssetReference(Archive, "MeshAssetGuid", "MeshAssetPath", Object.mMeshReference);
}
