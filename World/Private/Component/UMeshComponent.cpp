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

const UMesh* FMeshPickingProxy::GetMesh() const {
    return Source != nullptr ? Source->Mesh : nullptr;
}

void FMeshPickingProxy::Update(const UMesh* InMesh, const FTransform& Transform) {
    Source = InMesh != nullptr ? InMesh->GetPickingSource() : nullptr;

    const FVector3& Scale{Transform.GetScale()};

    Valid = std::isfinite(Scale.mX) && std::isfinite(Scale.mY) && std::isfinite(Scale.mZ) && Scale.mX != 0.0f && Scale.mY != 0.0f && Scale.mZ != 0.0f;

    if (!Valid)
        return;

    ReverseWinding = (Scale.mX < 0.0f) ^ (Scale.mY < 0.0f) ^ (Scale.mZ < 0.0f);
    InverseScale = {1.0f / Scale.mX, 1.0f / Scale.mY, 1.0f / Scale.mZ};
    DirectX::XMStoreFloat3(&Position, Transform.GetPosition().ToSimpleMath());
    DirectX::XMStoreFloat4(&Rotation, DirectX::XMQuaternionNormalize(Transform.GetRotationQuaternion().ToSimpleMath()));
}

bool UMeshComponent::RaycastMesh(const FRay& Ray, float& OutDistance, float MaxDistance) const {
    const UMesh* Mesh{ResolveMesh()};
    const Uint64 Revision{GetTransformRevision()};

    if (!mRaycastTransformInitialized || mRaycastTransformRevision != Revision || mRaycastProxy.GetMesh() != Mesh) {
        mRaycastProxy.Update(Mesh, GetComponentTransform());
        mRaycastTransformRevision = Revision;
        mRaycastTransformInitialized = true;
    }

    return mRaycastProxy.Raycast(Ray, OutDistance, MaxDistance);
}

bool FMeshPickingProxy::PrepareRay(const FRay& Ray, FRay& LocalRay, double& DirectionLength) const {
    if (!Valid)
        return false;

    const auto Scale{DirectX::XMLoadFloat3(&InverseScale)};
    const auto Q{DirectX::XMLoadFloat4(&Rotation)};
    const auto Origin{DirectX::XMVectorMultiply(DirectX::XMVector3InverseRotate(DirectX::XMVectorSubtract(Ray.position, DirectX::XMLoadFloat3(&Position)), Q), Scale)};
    const auto Direction{DirectX::XMVectorMultiply(DirectX::XMVector3InverseRotate(Ray.direction, Q), Scale)};
    const double DX{DirectX::XMVectorGetX(Direction)};
    const double DY{DirectX::XMVectorGetY(Direction)};
    const double DZ{DirectX::XMVectorGetZ(Direction)};

    DirectionLength = std::sqrt(DX * DX + DY * DY + DZ * DZ);

    if (!std::isfinite(DirectionLength) || DirectionLength <= 0.0 || DirectX::XMVector3IsNaN(Origin) || DirectX::XMVector3IsInfinite(Origin))
        return false;

    LocalRay = FRay{Origin, DirectX::XMVectorSet(static_cast<float>(DX / DirectionLength), static_cast<float>(DY / DirectionLength), static_cast<float>(DZ / DirectionLength), 0.0f)};

    return true;
}

bool FMeshPickingProxy::RaycastPrepared(const FRay& LocalRay, double DirectionLength, float& OutDistance, float MaxDistance) const {
    if (Source == nullptr || !(MaxDistance >= 0.0f))
        return false;

    const double Limit{static_cast<double>(MaxDistance) * DirectionLength};
    const float LocalLimit{Limit >= std::numeric_limits<float>::max() ? std::numeric_limits<float>::max() : std::bit_cast<float>(std::bit_cast<Uint32>((std::max)(0.0f, static_cast<float>(Limit))) + 1u)};
    float Distance{};

    if (!Source->Raycast(LocalRay, Distance, LocalLimit, ReverseWinding))
        return false;

    const float WorldDistance{static_cast<float>(Distance / DirectionLength)};

    if (WorldDistance > MaxDistance)
        return false;

    OutDistance = WorldDistance;

    return true;
}

bool FMeshPickingProxy::Raycast(const FRay& Ray, float& OutDistance, float MaxDistance) const {
    FRay LocalRay{};
    double DirectionLength{};

    return PrepareRay(Ray, LocalRay, DirectionLength) && RaycastPrepared(LocalRay, DirectionLength, OutDistance, MaxDistance);
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
