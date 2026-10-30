#include "pch.h"
#include "World/FWorldRaycast.h"
#include <cmath>

bool FWorldRaycastQuery::Excludes(const UPrimitiveComponent& Component) const {
    const AActor* Owner{Component.GetOwner()};

    if (Owner == nullptr || Owner->IsBeingDestroyed()) {
        return true;
    }

    for (const TObjectRef<AActor>& Excluded : mExcludedActorHierarchies) {
        const AActor* Actor{Excluded.Get()};

        if (Actor == nullptr) {
            continue;
        }

        if (Owner == Actor) {
            return true;
        }

        for (const USceneComponent* Parent{Owner->GetRootComponent()}; Parent != nullptr; Parent = Parent->GetParent()) {
            if (Parent->GetOwner() == Actor) {
                return true;
            }
        }
    }

    return false;
}

bool FWorldRaycastQuery::PrepareRay(const FRay& Ray, FRay& OutRay) const {
    const double X{Ray.direction.x};
    const double Y{Ray.direction.y};
    const double Z{Ray.direction.z};
    const double Length{std::sqrt(X * X + Y * Y + Z * Z)};

    if (!(mMaxDistance >= 0.0f) || !std::isfinite(Length) || Length <= 0.0 || !std::isfinite(Ray.position.x) || !std::isfinite(Ray.position.y) || !std::isfinite(Ray.position.z)) {
        return false;
    }

    OutRay = FRay{Ray.position, DirectX::SimpleMath::Vector3{static_cast<float>(X / Length), static_cast<float>(Y / Length), static_cast<float>(Z / Length)}};

    return true;
}

bool RaycastBox(const DirectX::BoundingOrientedBox& Box, const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) {
    float Distance{};

    if (!Box.Intersects(Ray.position, Ray.direction, Distance) || Distance > MaxDistance) {
        return false;
    }

    const auto Rotation{DirectX::XMLoadFloat4(&Box.Orientation)};
    const FVector3 Position{FVector3{Ray.position} + FVector3{Ray.direction} * Distance};
    DirectX::XMFLOAT3 Local{};

    DirectX::XMStoreFloat3(&Local, DirectX::XMVector3InverseRotate(DirectX::XMVectorSubtract(Position.ToSimpleMath(), DirectX::XMLoadFloat3(&Box.Center)), Rotation));

    const float Coordinates[]{Local.x, Local.y, Local.z};
    const float Extents[]{Box.Extents.x, Box.Extents.y, Box.Extents.z};
    Uint32 Axis{};

    for (Uint32 Candidate{1}; Candidate < 3; ++Candidate) {
        if (std::abs(std::abs(Coordinates[Candidate]) - Extents[Candidate]) < std::abs(std::abs(Coordinates[Axis]) - Extents[Axis])) {
            Axis = Candidate;
        }
    }

    float Normal[3]{};
    DirectX::XMFLOAT3 WorldNormal{};

    Normal[Axis] = Coordinates[Axis] < 0.0f ? -1.0f : 1.0f;
    DirectX::XMStoreFloat3(&WorldNormal, DirectX::XMVector3Rotate(DirectX::XMVectorSet(Normal[0], Normal[1], Normal[2], 0.0f), Rotation));
    OutHit.mDistance = Distance;
    OutHit.mPosition = Position;
    OutHit.mNormal = FVector3{WorldNormal};
    OutHit.mFaceIndex = std::numeric_limits<Uint32>::max();

    return true;
}
