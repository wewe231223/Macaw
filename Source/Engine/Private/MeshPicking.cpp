/**
 * @file MeshPicking.cpp
 * @brief 월드 공간 광선과 CPU 메시 삼각형의 교차 검사에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/MeshPicking.h"
#include "Engine/FMeshData.h"
#include "Core/Math.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FMatrix.h"

bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance) {
    if (Mesh == nullptr || !Mesh->GetBounds().Transformed(World).Intersects(Ray, Distance)) {
        return false;
    }

    FMatrix Inverse{};
    if (!World.TryInverse(Inverse)) {
        return false;
    }

    const FRay Local{Inverse.TransformPosition(Ray.mOrigin), Inverse.TransformVector(Ray.mDirection)};
    bool Found{};
    for (std::size_t Index{}; Index + 2 < Mesh->GetIndices().size(); Index += 3) {
        const FVector A{Mesh->GetPositions()[Mesh->GetIndices()[Index]]};
        const FVector B{Mesh->GetPositions()[Mesh->GetIndices()[Index + 1]]};
        const FVector C{Mesh->GetPositions()[Mesh->GetIndices()[Index + 2]]};
        float HitDistance{};
        if (Math::IntersectTriangle(Local, A, B, C, HitDistance) && HitDistance < Distance) {
            Distance = HitDistance;
            Found = true;
        }
    }
    return Found;
}
