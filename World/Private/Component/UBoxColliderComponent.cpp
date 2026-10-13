#include "pch.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Debug/ILineDrawContext.h"
#include <cfloat>
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/AActor.h"
#include "Asset/UMesh.h"
#include "CoreUObject/UObjectSystem.h"

#include <array>
#include <cmath>

UBoxColliderComponent::UBoxColliderComponent() {
    InitializeClass(StaticClass());
    SetWantsInitializeComponent(true);
}

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
    const UMesh* Asset{Mesh != nullptr ? Mesh->ResolveMesh() : nullptr};

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
    DirectX::BoundingOrientedBox OrientedBounds{};

    DirectX::BoundingOrientedBox::CreateFromBoundingBox(OrientedBounds, Bounds);
    mBounds.SetNative(OrientedBounds);
    SetPickingBox(OrientedBounds);

    return true;
}

bool UBoxColliderComponent::RaycastBounds(const FRay& Ray, float& OutDistance) const {
    DirectX::BoundingOrientedBox WorldBox{};

    mBounds.ToNative().Transform(WorldBox, GetComponentToWorld().ToSimpleMath());

    return WorldBox.Intersects(Ray.position, Ray.direction, OutDistance);
}

FVector3 UBoxColliderComponent::GetExtent() const {
    return mBounds.mExtent;
}

void UBoxColliderComponent::SetExtent(const FVector3& InExtent) {
    FBoxColliderBounds Bounds{mBounds};

    Bounds.mExtent = InExtent;
    SetBounds(Bounds);
}

bool UBoxColliderComponent::ResolveLoadedReferences() {
    if (!UCollisionComponent::ResolveLoadedReferences()) {
        return false;
    }

    if (mPendingMeshComponentGuid.IsValid()) {
        UObject* Object{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mPendingMeshComponentGuid))};

        if (Object == nullptr || !Object->GetClass()->IsChildOf(UMeshComponent::StaticClass())) {
            return false;
        }

        mMeshComponent.Set(static_cast<UMeshComponent*>(Object));
        mPendingMeshComponentGuid = {};
    }

    BuildBoundsFromMesh();

    return true;
}

void UBoxColliderComponent::InitializeComponent() {
    Super::InitializeComponent();
    BuildBoundsFromMesh();
}

void UBoxColliderComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        SetBounds(mBounds);
    }
}

void UBoxColliderComponent::DrawEditorBounds(ILineDrawContext* LineContext, ELineDepthMode DepthMode) const {
    DirectX::BoundingOrientedBox WorldBox{};

    mBounds.ToNative().Transform(WorldBox, GetComponentToWorld().ToSimpleMath());

    std::array<DirectX::XMFLOAT3, DirectX::BoundingOrientedBox::CORNER_COUNT> Corners{};

    WorldBox.GetCorners(Corners.data());

    const FVector4 LineColor{FVector4{1.0f, 1.0f, 0.0f, 1.0f}};
    const float Thickness{1.0f};

    const auto AddEdge{[LineContext, &Corners, LineColor, Thickness, DepthMode](std::size_t Start, std::size_t End) {
        LineContext->AddLine(FVector3{Corners[Start]}, FVector3{Corners[End]}, LineColor, Thickness, DepthMode);
    }};

    AddEdge(0, 1);
    AddEdge(1, 2);
    AddEdge(2, 3);
    AddEdge(3, 0);
    AddEdge(4, 5);
    AddEdge(5, 6);
    AddEdge(6, 7);
    AddEdge(7, 4);
    AddEdge(0, 4);
    AddEdge(1, 5);
    AddEdge(2, 6);
    AddEdge(3, 7);
}

DirectX::BoundingOrientedBox FBoxColliderBounds::ToNative() const {
    return DirectX::BoundingOrientedBox{mCenter.ToSimpleMath(), mExtent.ToSimpleMath(), mOrientation.ToSimpleMath()};
}

void FBoxColliderBounds::SetNative(const DirectX::BoundingOrientedBox& Bounds) {
    mCenter = FVector3{Bounds.Center};
    mExtent = FVector3{Bounds.Extents};
    mOrientation = FQuat{Bounds.Orientation};
}

const FBoxColliderBounds& UBoxColliderComponent::GetBounds() const {
    return mBounds;
}

void UBoxColliderComponent::SetBounds(const FBoxColliderBounds& Bounds) {
    if (!std::isfinite(Bounds.mExtent.mX) || !std::isfinite(Bounds.mExtent.mY) || !std::isfinite(Bounds.mExtent.mZ)) {
        return;
    }

    mBounds = Bounds;
    mBounds.mExtent = FVector3{(std::max)(0.001f, Bounds.mExtent.mX), (std::max)(0.001f, Bounds.mExtent.mY), (std::max)(0.001f, Bounds.mExtent.mZ)};
    SetPickingBox(mBounds.ToNative());
}

void UBoxColliderComponent::SerializeMeshComponentProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UBoxColliderComponent& Object{*static_cast<UBoxColliderComponent*>(Container)};

    SerializeObjectReference(Archive, Property.GetSerializationName(), Object.GetMeshComponent(), Object.mPendingMeshComponentGuid);

    if (Archive.IsLoading()) {
        Object.mMeshComponent = {};
    }
}
