#include "Engine/PrimitiveMesh.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"
#include "Engine/FMeshData.h"
#include "Engine/EPrimitiveType.h"

#include <cmath>
#include <numbers>

FString PrimitiveName(EPrimitiveType Type) {
    switch (Type) {
    case EPrimitiveType::Plane:
        return "Plane";

    case EPrimitiveType::Cube:
        return "Cube";

    case EPrimitiveType::Sphere:
        return "Sphere";

    default:
        return "Unknown";
    }
}

FMeshData BuildPrimitive(EPrimitiveType Type) {
    FMeshData Mesh{};

    if (Type == EPrimitiveType::Plane) {
        Mesh.mVertices = {{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
        Mesh.mIndices = {0, 1, 2, 0, 2, 3};

        Mesh.mBounds = {{-0.5f, -0.5f, -0.001f}, {0.5f, 0.5f, 0.001f}};
    } else if (Type == EPrimitiveType::Cube) {
        const std::array<FVector, 6> Normals{{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}};

        for (const FVector& Normal : Normals) {
            const FVector Up{std::abs(Normal.z) > 0.5f ? FVector{0.0f, 1.0f, 0.0f} : FVector{0.0f, 0.0f, 1.0f}};
            const FVector Right{Up.Cross(Normal)};
            const Uint32 Base{static_cast<Uint32>(Mesh.mVertices.size())};

            Mesh.mVertices.push_back({(Normal - Right - Up) * 0.5f, Normal});
            Mesh.mVertices.push_back({(Normal + Right - Up) * 0.5f, Normal});
            Mesh.mVertices.push_back({(Normal + Right + Up) * 0.5f, Normal});
            Mesh.mVertices.push_back({(Normal - Right + Up) * 0.5f, Normal});

            Mesh.mIndices.insert(Mesh.mIndices.end(), {Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
        }

        Mesh.mBounds = {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    } else if (Type == EPrimitiveType::Sphere) {
        constexpr Uint32 Rings{20};
        constexpr Uint32 Segments{32};

        std::array<FVector2, Segments + 1> Circle{};

        for (Uint32 Segment{}; Segment < Segments; ++Segment) {
            Circle[Segment] = FVector2{1.0f, 0.0f}.Rotated(360.0f * Segment / Segments);
        }

        Circle.back() = Circle.front();

        Mesh.mVertices.reserve((Rings + 1) * (Segments + 1));

        for (Uint32 Ring{}; Ring <= Rings; ++Ring) {
            const float Phi{std::numbers::pi_v<float> * static_cast<float>(Ring) / Rings};
            const float Radius{std::sin(Phi)};
            const float Height{std::cos(Phi)};

            for (Uint32 Segment{}; Segment <= Segments; ++Segment) {
                const FVector2 Radial{Circle[Segment] * Radius};
                const FVector Normal{Radial.x, Radial.y, Height};

                Mesh.mVertices.push_back({Normal * 0.5f, Normal});
            }
        }

        for (Uint32 Ring{}; Ring < Rings; ++Ring) {
            for (Uint32 Segment{}; Segment < Segments; ++Segment) {
                const Uint32 A{Ring * (Segments + 1) + Segment};
                const Uint32 B{A + Segments + 1};

                Mesh.mIndices.insert(Mesh.mIndices.end(), {A, B, A + 1, A + 1, B, B + 1});
            }
        }

        Mesh.mBounds = {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
    }

    return Mesh;
}

const FMeshData& GetPrimitiveMesh(EPrimitiveType Type) {
    static const std::array<FMeshData, 3> Meshes{BuildPrimitive(EPrimitiveType::Plane), BuildPrimitive(EPrimitiveType::Cube), BuildPrimitive(EPrimitiveType::Sphere)};
    return Meshes[static_cast<std::size_t>(Type)];
}
