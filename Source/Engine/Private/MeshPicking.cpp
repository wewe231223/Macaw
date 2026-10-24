#include "Engine/MeshPicking.h"
#include "Engine/FMeshData.h"
#include "Core/Math.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FMatrix.h"

bool RaycastMesh(const FMeshData* Mesh, const FMatrix& World, const FRay& Ray, float& Distance, FVector* Normal) {
    if (Mesh == nullptr || !Mesh->mBounds.Transformed(World).Intersects(Ray, Distance)) {
        return false;
    }

    FMatrix Inverse{};
    if (!World.TryInverse(Inverse)) {
        return false;
    }

    const FRay Local{Inverse.TransformPosition(Ray.mOrigin), Inverse.TransformVector(Ray.mDirection)};
    bool Found{};
    for (std::size_t Index{}; Index + 2 < Mesh->mIndices.size(); Index += 3) {
        const FVector A{Mesh->mVertices[Mesh->mIndices[Index]].mPosition};
        const FVector B{Mesh->mVertices[Mesh->mIndices[Index + 1]].mPosition};
        const FVector C{Mesh->mVertices[Mesh->mIndices[Index + 2]].mPosition};
        float HitDistance{};
        if (Math::IntersectTriangle(Local, A, B, C, HitDistance) && HitDistance < Distance) {
            Distance = HitDistance;
            Found = true;
            if (Normal != nullptr) {
                *Normal = Inverse.Transposed().TransformVector((B - A).Cross(C - A)).Normalized();
                if (Normal->Dot(Ray.mDirection) > 0.0f) {
                    *Normal = -*Normal;
                }
            }
        }
    }
    return Found;
}
