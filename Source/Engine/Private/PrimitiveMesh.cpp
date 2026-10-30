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

namespace Engine {

    /**
     * @brief 프리미티브 종류의 표시 이름을 반환합니다.
     */
    Core::FString PrimitiveName(Engine::EPrimitiveType Type) {
        switch (Type) {
        case Engine::EPrimitiveType::Plane:
            return "Plane";

        case Engine::EPrimitiveType::Cube:
            return "Cube";

        case Engine::EPrimitiveType::Sphere:
            return "Sphere";

        default:
            return "Unknown";
        }
    }

    /**
     * @brief 기본 프리미티브의 CPU 정점, 삼각형 인덱스와 경계 상자를 생성합니다.
     */
    Engine::FMeshData BuildPrimitive(Engine::EPrimitiveType Type) {
        Engine::FMeshData Mesh{};
        Core::TArray<Core::FVector>& Positions{Mesh.GetPositions()};
        Core::TArray<Core::FVector>& Normals{Mesh.GetNormals()};
        Core::TArray<Core::FVector4>& Tangents{Mesh.GetTangents()};
        Core::TArray<Core::Uint32>& Indices{Mesh.GetIndices()};
        Mesh.GetUVChannels().resize(1);

        Core::TArray<Core::FVector2>& UVs{Mesh.GetUVChannels().front()};

        if (Type == Engine::EPrimitiveType::Plane) {
            Positions = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}};
            Normals.assign(4, {0.0f, 0.0f, 1.0f});
            Tangents.assign(4, {1.0f, 0.0f, 0.0f, 1.0f});
            UVs = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
            Indices = {0, 1, 2, 0, 2, 3};

            Mesh.SetBounds({{-0.5f, -0.5f, -0.001f}, {0.5f, 0.5f, 0.001f}});
        } else if (Type == Engine::EPrimitiveType::Cube) {
            const std::array<Core::FVector, 6> FaceNormals{{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}};

            for (const Core::FVector& Normal : FaceNormals) {
                const Core::FVector Up{std::abs(Normal.z) > 0.5f ? Core::FVector{0.0f, 1.0f, 0.0f} : Core::FVector{0.0f, 0.0f, 1.0f}};
                const Core::FVector Right{Up.Cross(Normal)};
                const Core::Uint32 Base{static_cast<Core::Uint32>(Positions.size())};

                Positions.insert(Positions.end(), {(Normal - Right - Up) * 0.5f, (Normal + Right - Up) * 0.5f, (Normal + Right + Up) * 0.5f, (Normal - Right + Up) * 0.5f});
                Normals.insert(Normals.end(), 4, Normal);
                Tangents.insert(Tangents.end(), 4, Core::FVector4{Right, 1.0f});
                UVs.insert(UVs.end(), {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}});

                Indices.insert(Indices.end(), {Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
            }

            Mesh.SetBounds({{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}});
        } else if (Type == Engine::EPrimitiveType::Sphere) {
            constexpr Core::Uint32 Rings{20};
            constexpr Core::Uint32 Segments{32};

            std::array<Core::FVector2, Segments + 1> Circle{};

            for (Core::Uint32 Segment{}; Segment < Segments; ++Segment) {
                Circle[Segment] = Core::FVector2{1.0f, 0.0f}.Rotated(360.0f * Segment / Segments);
            }

            Circle.back() = Circle.front();

            Positions.reserve((Rings + 1) * (Segments + 1));
            Normals.reserve((Rings + 1) * (Segments + 1));
            Tangents.reserve((Rings + 1) * (Segments + 1));
            UVs.reserve((Rings + 1) * (Segments + 1));

            for (Core::Uint32 Ring{}; Ring <= Rings; ++Ring) {
                const float Phi{std::numbers::pi_v<float> * static_cast<float>(Ring) / Rings};
                const float Radius{std::sin(Phi)};
                const float Height{std::cos(Phi)};

                for (Core::Uint32 Segment{}; Segment <= Segments; ++Segment) {
                    const Core::FVector2 Radial{Circle[Segment] * Radius};
                    const Core::FVector Normal{Radial.x, Radial.y, Height};

                    Positions.push_back(Normal * 0.5f);
                    Normals.push_back(Normal);
                    Tangents.push_back({-Circle[Segment].y, Circle[Segment].x, 0.0f, -1.0f});
                    UVs.push_back({static_cast<float>(Segment) / Segments, static_cast<float>(Ring) / Rings});
                }
            }

            for (Core::Uint32 Ring{}; Ring < Rings; ++Ring) {
                for (Core::Uint32 Segment{}; Segment < Segments; ++Segment) {
                    const Core::Uint32 A{Ring * (Segments + 1) + Segment};
                    const Core::Uint32 B{A + Segments + 1};

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
    const Engine::FMeshData& GetPrimitiveMesh(Engine::EPrimitiveType Type) {
        static const std::array<Engine::FMeshData, 3> Meshes{Engine::BuildPrimitive(Engine::EPrimitiveType::Plane), Engine::BuildPrimitive(Engine::EPrimitiveType::Cube), Engine::BuildPrimitive(Engine::EPrimitiveType::Sphere)};
        return Meshes[static_cast<std::size_t>(Type)];
    }

}
