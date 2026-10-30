/**
 * @file PrimitiveMesh.cpp
 * @brief Plane, Cube와 Sphere 기본 메시의 생성 및 캐시 조회를 제공합니다.
 * @ingroup Engine
 */

#include "Engine/PrimitiveMesh.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FVector2.h"
#include "Engine/FMeshData.h"
#include "Engine/EPrimitiveType.h"

#include <cmath>
#include <numbers>

/**
 * @brief 프리미티브 종류의 표시 이름을 반환합니다.
 */
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

/**
 * @brief 기본 프리미티브의 CPU 정점, 삼각형 인덱스와 경계 상자를 생성합니다.
 */
FMeshData BuildPrimitive(EPrimitiveType Type) {
    FMeshData Mesh{};
    TArray<FVector>& Positions{Mesh.GetPositions()};
    TArray<FVector>& Normals{Mesh.GetNormals()};
    TArray<FVector4>& Tangents{Mesh.GetTangents()};
    TArray<Uint32>& Indices{Mesh.GetIndices()};
    Mesh.GetUVChannels().resize(1);

    TArray<FVector2>& UVs{Mesh.GetUVChannels().front()};

    if (Type == EPrimitiveType::Plane) {
        Positions = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}};
        Normals.assign(4, {0.0f, 0.0f, 1.0f});
        Tangents.assign(4, {1.0f, 0.0f, 0.0f, 1.0f});
        UVs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
        Indices = {0, 1, 2, 0, 2, 3};

        Mesh.SetBounds({{-0.5f, -0.5f, -0.001f}, {0.5f, 0.5f, 0.001f}});
    } else if (Type == EPrimitiveType::Cube) {
        const std::array<FVector, 6> FaceNormals{{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}};

        for (const FVector& Normal : FaceNormals) {
            const FVector Up{std::abs(Normal.z) > 0.5f ? FVector{0.0f, 1.0f, 0.0f} : FVector{0.0f, 0.0f, 1.0f}};
            const FVector Right{Up.Cross(Normal)};
            const Uint32 Base{static_cast<Uint32>(Positions.size())};

            Positions.insert(Positions.end(), {(Normal - Right - Up) * 0.5f, (Normal + Right - Up) * 0.5f, (Normal + Right + Up) * 0.5f, (Normal - Right + Up) * 0.5f});
            Normals.insert(Normals.end(), 4, Normal);
            Tangents.insert(Tangents.end(), 4, FVector4{Right, 1.0f});
            UVs.insert(UVs.end(), {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}});

            Indices.insert(Indices.end(), {Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
        }

        Mesh.SetBounds({{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}});
    } else if (Type == EPrimitiveType::Sphere) {
        constexpr Uint32 Rings{20};
        constexpr Uint32 Segments{32};

        std::array<FVector2, Segments + 1> Circle{};

        for (Uint32 Segment{}; Segment < Segments; ++Segment) {
            Circle[Segment] = FVector2{1.0f, 0.0f}.Rotated(360.0f * Segment / Segments);
        }

        Circle.back() = Circle.front();

        Positions.reserve((Rings + 1) * (Segments + 1));
        Normals.reserve((Rings + 1) * (Segments + 1));
        Tangents.reserve((Rings + 1) * (Segments + 1));
        UVs.reserve((Rings + 1) * (Segments + 1));

        for (Uint32 Ring{}; Ring <= Rings; ++Ring) {
            const float Phi{std::numbers::pi_v<float> * static_cast<float>(Ring) / Rings};
            const float Radius{std::sin(Phi)};
            const float Height{std::cos(Phi)};

            for (Uint32 Segment{}; Segment <= Segments; ++Segment) {
                const FVector2 Radial{Circle[Segment] * Radius};
                const FVector Normal{Radial.x, Radial.y, Height};

                Positions.push_back(Normal * 0.5f);
                Normals.push_back(Normal);
                Tangents.push_back({-Circle[Segment].y, Circle[Segment].x, 0.0f, -1.0f});
                UVs.push_back({static_cast<float>(Segment) / Segments, static_cast<float>(Ring) / Rings});
            }
        }

        for (Uint32 Ring{}; Ring < Rings; ++Ring) {
            for (Uint32 Segment{}; Segment < Segments; ++Segment) {
                const Uint32 A{Ring * (Segments + 1) + Segment};
                const Uint32 B{A + Segments + 1};

                Indices.insert(Indices.end(), {A, B, A + 1, A + 1, B, B + 1});
            }
        }

        Mesh.SetBounds({{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}});
    }

    Mesh.GetColors().assign(Positions.size(), {1.0f, 1.0f, 1.0f, 1.0f});

    return Mesh;
}

/**
 * @brief 종류별로 캐시된 읽기 전용 기본 메시를 반환합니다.
 */
const FMeshData& GetPrimitiveMesh(EPrimitiveType Type) {
    static const std::array<FMeshData, 3> Meshes{BuildPrimitive(EPrimitiveType::Plane), BuildPrimitive(EPrimitiveType::Cube), BuildPrimitive(EPrimitiveType::Sphere)};
    return Meshes[static_cast<std::size_t>(Type)];
}
