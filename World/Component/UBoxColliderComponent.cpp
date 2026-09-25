#include "pch.h"
#include "UBoxColliderComponent.h"

#include "UMeshComponent.h"
#include "World/AActor.h"
#include "Asset/UMesh.h"
#include "Core/Base/UObjectSystem.h"

#include <array>

void UBoxColliderComponent::SetMeshComponent(UMeshComponent* InMeshComponent) {
    mMeshComponent.Set(InMeshComponent);
    mPendingMeshComponentGuid = {};
    BuildBoundsFromMesh();
}

UMeshComponent* UBoxColliderComponent::GetMeshComponent() const {
    return mMeshComponent.Get();
}

bool UBoxColliderComponent::BuildBoundsFromMesh() {
    UMeshComponent* Mesh{mMeshComponent.Get()};
    UMesh* Asset{Mesh != nullptr ? Mesh->ResolveMesh() : nullptr};
    if (Asset == nullptr) {
        return false;
    }

    const auto Positions{Asset->GetVertexAttributeData<EVertexAttribute::Position>()};
    if (Positions.empty()) {
        return false;
    }

    std::vector<DirectX::XMFLOAT3> Points{};
    Points.reserve(Positions.size());
    for (const FVector3& Position : Positions) {
        Points.emplace_back(Position.mX, Position.mY, Position.mZ);
    }

    DirectX::BoundingBox Bounds{};
    DirectX::BoundingBox::CreateFromPoints(Bounds, Points.size(), Points.data(), sizeof(DirectX::XMFLOAT3));
    DirectX::BoundingOrientedBox::CreateFromBoundingBox(mObb, Bounds);
    SetPickingBox(mObb);
    return true;
}

bool UBoxColliderComponent::RaycastBounds(const FRay& Ray, float& OutDistance) const {
    DirectX::BoundingOrientedBox WorldBox{};
    mObb.Transform(WorldBox, GetComponentToWorld().ToSimpleMath());
    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

FVector3 UBoxColliderComponent::GetExtent() const {
    return FVector3{mObb.Extents.x, mObb.Extents.y, mObb.Extents.z};
}

void UBoxColliderComponent::SetExtent(const FVector3& InExtent) {
    mObb.Extents = DirectX::XMFLOAT3(InExtent.mX, InExtent.mY, InExtent.mZ);
    SetPickingBox(mObb);
}

bool UBoxColliderComponent::ResolveLoadedReferences() {
    if (!UCollisionComponent::ResolveLoadedReferences()) {
        return false;
    }
    if (mPendingMeshComponentGuid.IsValid()) {
        UObject* Object{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingMeshComponentGuid))};
        if (Object == nullptr || !Object->GetTypeInfo()->IsA(UMeshComponent::StaticTypeInfo())) {
            return false;
        }
        mMeshComponent.Set(static_cast<UMeshComponent*>(Object));
        mPendingMeshComponentGuid = {};
    }
    BuildBoundsFromMesh();
    return true;
}

void UBoxColliderComponent::InitializeComponent() {
    UCollisionComponent::InitializeComponent();
    BuildBoundsFromMesh();
}

void UBoxColliderComponent::Serialize(FArchive& Archive) {
    UCollisionComponent::Serialize(Archive);

    FString MeshComponentGuid{};
    if (UMeshComponent * Mesh{mMeshComponent.Get()}) {
        MeshComponentGuid = Mesh->GetGuid().ToString();
    }
    Archive.Serialize("GuidMeshComponent", MeshComponentGuid);
    if (Archive.IsLoading() && !MeshComponentGuid.empty() && !mPendingMeshComponentGuid.Parse(MeshComponentGuid)) {
        mPendingMeshComponentGuid = {};
    }

    FVector3 Center{mObb.Center};
    FVector3 Extent{mObb.Extents};
    FQuat Orientation{mObb.Orientation};
    Archive.Serialize("OBB_Center", Center);
    Archive.Serialize("OBB_Extent", Extent);
    Archive.Serialize("OBB_Orientation", Orientation);
    if (Archive.IsLoading()) {
        mObb.Center = Center.ToSimpleMath();
        mObb.Extents = Extent.ToSimpleMath();
        mObb.Orientation = Orientation.ToSimpleMath();
        SetPickingBox(mObb);
    }
}
