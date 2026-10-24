#include "Engine/EditorMesh.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/EPrimitiveType.h"
#include <cmath>
#include <numbers>
#include <limits>

static void AddTriangle(FMeshData* Mesh, const FVector& A, const FVector& B, const FVector& C) {
    const Uint32 Base{static_cast<Uint32>(Mesh->mVertices.size())};
    const FVector Normal{(B - A).Cross(C - A).Normalized()};
    Mesh->mVertices.insert(Mesh->mVertices.end(), {{A, Normal}, {B, Normal}, {C, Normal}});
    Mesh->mIndices.insert(Mesh->mIndices.end(), {Base, Base + 1, Base + 2});
}

static void AddBox(FMeshData* Mesh, const FVector& Minimum, const FVector& Maximum) {
    const FMeshData* Cube{&GetPrimitiveMesh(EPrimitiveType::Cube)};
    const FVector Size{Maximum - Minimum};
    const FVector Center{(Maximum + Minimum) * 0.5f};
    for (std::size_t Index{}; Index < Cube->mIndices.size(); Index += 3) {
        AddTriangle(Mesh, Cube->mVertices[Cube->mIndices[Index]].mPosition * Size + Center, Cube->mVertices[Cube->mIndices[Index + 1]].mPosition * Size + Center, Cube->mVertices[Cube->mIndices[Index + 2]].mPosition * Size + Center);
    }
}

static FMeshData BuildMesh(EEditorMesh Type) {
    FMeshData Mesh{};
    constexpr float Pi{std::numbers::pi_v<float>};
    if (Type == EEditorMesh::Arrow) {
        for (Uint32 Index{}; Index < 16; ++Index) {
            const float A{2.0f * Pi * Index / 16.0f};
            const float B{2.0f * Pi * (Index + 1) / 16.0f};
            const FVector BottomA{std::cos(A) * 2.0f, std::sin(A) * 2.0f, 11.0f};
            const FVector BottomB{std::cos(B) * 2.0f, std::sin(B) * 2.0f, 11.0f};
            const FVector TopA{BottomA.x, BottomA.y, 72.5f};
            const FVector TopB{BottomB.x, BottomB.y, 72.5f};
            AddTriangle(&Mesh, BottomA, BottomB, TopA);
            AddTriangle(&Mesh, TopA, BottomB, TopB);
            AddTriangle(&Mesh, {0.0f, 0.0f, 11.0f}, BottomB, BottomA);
            const FVector ConeA{std::cos(A) * 6.5f, std::sin(A) * 6.5f, 72.5f};
            const FVector ConeB{std::cos(B) * 6.5f, std::sin(B) * 6.5f, 72.5f};
            AddTriangle(&Mesh, ConeA, ConeB, {0.0f, 0.0f, 87.5f});
            AddTriangle(&Mesh, {0.0f, 0.0f, 72.5f}, ConeB, ConeA);
        }
    } else if (Type == EEditorMesh::Hammer) {
        AddBox(&Mesh, {-2.0f, -2.0f, 11.0f}, {2.0f, 2.0f, 75.5f});
        AddBox(&Mesh, {-5.5f, -5.5f, 75.5f}, {5.5f, 5.5f, 84.5f});
    } else if (Type == EEditorMesh::Ring || Type == EEditorMesh::HalfRing) {
        const Uint32 Segments{Type == EEditorMesh::Ring ? 96u : 48u};
        for (Uint32 Segment{}; Segment < Segments; ++Segment) {
            const float A{Type == EEditorMesh::Ring ? Segment * 2.0f * Pi / Segments : -Pi * 0.5f + Segment * Pi / Segments};
            const float B{Type == EEditorMesh::Ring ? (Segment + 1) * 2.0f * Pi / Segments : -Pi * 0.5f + (Segment + 1) * Pi / Segments};
            for (Uint32 Side{}; Side < 8; ++Side) {
                const float C{Side * 2.0f * Pi / 8.0f};
                const float D{(Side + 1) * 2.0f * Pi / 8.0f};
                const FVector P{(74.0f + std::cos(C) * 2.0f) * std::cos(A), (74.0f + std::cos(C) * 2.0f) * std::sin(A), std::sin(C) * 2.0f};
                const FVector Q{(74.0f + std::cos(D) * 2.0f) * std::cos(A), (74.0f + std::cos(D) * 2.0f) * std::sin(A), std::sin(D) * 2.0f};
                const FVector R{(74.0f + std::cos(C) * 2.0f) * std::cos(B), (74.0f + std::cos(C) * 2.0f) * std::sin(B), std::sin(C) * 2.0f};
                const FVector S{(74.0f + std::cos(D) * 2.0f) * std::cos(B), (74.0f + std::cos(D) * 2.0f) * std::sin(B), std::sin(D) * 2.0f};
                AddTriangle(&Mesh, P, Q, R);
                AddTriangle(&Mesh, R, Q, S);
            }
        }
    } else if (Type == EEditorMesh::Cube) {
        Mesh = GetPrimitiveMesh(EPrimitiveType::Cube);
    } else {
        AddTriangle(&Mesh, {}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        if (Type == EEditorMesh::Quad) {
            AddTriangle(&Mesh, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        }
    }
    const float Largest{std::numeric_limits<float>::max()};
    FVector Minimum{Largest, Largest, Largest};
    FVector Maximum{-Largest, -Largest, -Largest};
    for (const FMeshVertex& Vertex : Mesh.mVertices) {
        Minimum.x = std::min(Minimum.x, Vertex.mPosition.x);
        Minimum.y = std::min(Minimum.y, Vertex.mPosition.y);
        Minimum.z = std::min(Minimum.z, Vertex.mPosition.z);
        Maximum.x = std::max(Maximum.x, Vertex.mPosition.x);
        Maximum.y = std::max(Maximum.y, Vertex.mPosition.y);
        Maximum.z = std::max(Maximum.z, Vertex.mPosition.z);
    }
    Mesh.mBounds = {Minimum - FVector{0.001f, 0.001f, 0.001f}, Maximum + FVector{0.001f, 0.001f, 0.001f}};
    return Mesh;
}

const FMeshData& GetEditorMesh(EEditorMesh Type) {
    if (Type >= EEditorMesh::Count) {
        static const FMeshData Empty{};
        return Empty;
    }
    static const std::array<FMeshData, static_cast<std::size_t>(EEditorMesh::Count)> Meshes{BuildMesh(EEditorMesh::Arrow), BuildMesh(EEditorMesh::Hammer), BuildMesh(EEditorMesh::Ring), BuildMesh(EEditorMesh::HalfRing), BuildMesh(EEditorMesh::Quad), BuildMesh(EEditorMesh::Triangle), BuildMesh(EEditorMesh::Cube)};
    return Meshes[static_cast<std::size_t>(Type)];
}
