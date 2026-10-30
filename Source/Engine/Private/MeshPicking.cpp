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

/**
 * @brief 월드 변환을 적용한 메시의 모든 삼각형과 광선의 최근접 교차를 검사합니다.
 * @param[in] Mesh 정점·인덱스 데이터 또는 에디터 메시 요청입니다.
 * @param[in] World 메시의 월드 변환 또는 액터를 소유한 월드입니다.
 * @param[in] Ray 월드 공간 광선입니다.
 * @param[out] Distance 최근접 교차 거리의 출력입니다.
 * @return 최근접 교차를 찾으면 true이며 Distance에 월드 거리를 기록합니다.
 */
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
