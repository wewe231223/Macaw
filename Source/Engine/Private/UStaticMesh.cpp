/**
 * @file UStaticMesh.cpp
 * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/UStaticMesh.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

/**
 * @brief 외부 메시 파일을 가져오는 Assimp 타입을 참조합니다.
 * @namespace Assimp
 */
using namespace Assimp;
using namespace Microsoft::WRL;

UStaticMesh::UStaticMesh() = default;

UStaticMesh::UStaticMesh(const std::filesystem::path& MeshPath) {
    LoadFromFile(MeshPath);
}

UStaticMesh::~UStaticMesh() = default;

bool UStaticMesh::LoadFromFile(const std::filesystem::path& MeshPath) {
    if (MeshPath.empty()) {
        return false;
    }

    Importer MeshImporter{};
    MeshImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    MeshImporter.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
    MeshImporter.SetPropertyBool(AI_CONFIG_PP_FID_IGNORE_TEXTURECOORDS, true);
    const FString ImportPath{FilePath::ToUtf8(MeshPath)};
    constexpr Uint32 ImportFlags{aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_SortByPType | aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure};
    const aiScene* Scene{MeshImporter.ReadFile(ImportPath, ImportFlags)};

    if (Scene == nullptr || (Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !Scene->HasMeshes()) {
        return false;
    }

    constexpr std::size_t MaximumBytes{std::numeric_limits<Uint32>::max()};
    const std::size_t MaximumVertices{MaximumBytes / GetTangentStride()};
    constexpr std::size_t MaximumIndices{MaximumBytes / sizeof(Uint32)};
    std::size_t VertexCount{};
    std::size_t IndexCount{};
    Uint32 UVChannelCount{};

    for (Uint32 MeshIndex{}; MeshIndex < Scene->mNumMeshes; ++MeshIndex) {
        const aiMesh& SourceMesh{*Scene->mMeshes[MeshIndex]};

        if (!SourceMesh.HasPositions() || !SourceMesh.HasFaces() || SourceMesh.mNumVertices > MaximumVertices - VertexCount || SourceMesh.mNumFaces > (MaximumIndices - IndexCount) / 3) {
            return false;
        }

        VertexCount += SourceMesh.mNumVertices;
        IndexCount += static_cast<std::size_t>(SourceMesh.mNumFaces) * 3;

        for (Uint32 Channel{}; Channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++Channel) {
            if (SourceMesh.HasTextureCoords(Channel)) {
                UVChannelCount = std::max(UVChannelCount, Channel + 1);
            }
        }
    }

    const std::size_t UVStride{std::max<std::size_t>(1, UVChannelCount) * sizeof(float) * 2};

    if (VertexCount == 0 || IndexCount == 0 || UVStride > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES || VertexCount > MaximumBytes / UVStride) {
        return false;
    }

    FMeshData MeshData{};
    TArray<FVector>& Positions{MeshData.GetPositions()};
    TArray<FVector>& Normals{MeshData.GetNormals()};
    TArray<FVector4>& Tangents{MeshData.GetTangents()};
    TArray<TArray<FVector2>>& UVChannels{MeshData.GetUVChannels()};
    TArray<FVector4>& Colors{MeshData.GetColors()};
    TArray<Uint32>& Indices{MeshData.GetIndices()};
    Positions.reserve(VertexCount);
    Normals.reserve(VertexCount);
    Tangents.reserve(VertexCount);
    Colors.reserve(VertexCount);
    Indices.reserve(IndexCount);
    UVChannels.resize(UVChannelCount);

    for (TArray<FVector2>& Channel : UVChannels) {
        Channel.reserve(VertexCount);
    }

    for (Uint32 MaterialIndex{}; MaterialIndex < Scene->mNumMaterials; ++MaterialIndex) {
        aiString Name{};
        Scene->mMaterials[MaterialIndex]->Get(AI_MATKEY_NAME, Name);
        MeshData.GetMaterialSlots().emplace_back(Name.C_Str(), FString{});
    }

    for (Uint32 MeshIndex{}; MeshIndex < Scene->mNumMeshes; ++MeshIndex) {
        const aiMesh& SourceMesh{*Scene->mMeshes[MeshIndex]};
        const Uint32 BaseVertex{static_cast<Uint32>(Positions.size())};
        const Uint32 FirstIndex{static_cast<Uint32>(Indices.size())};

        for (Uint32 VertexIndex{}; VertexIndex < SourceMesh.mNumVertices; ++VertexIndex) {
            const aiVector3D& Position{SourceMesh.mVertices[VertexIndex]};
            Positions.push_back({Position.x, Position.y, Position.z});
            const FVector Normal{SourceMesh.HasNormals() ? FVector{SourceMesh.mNormals[VertexIndex].x, SourceMesh.mNormals[VertexIndex].y, SourceMesh.mNormals[VertexIndex].z}.Normalized() : FVector{0.0f, 0.0f, 1.0f}};
            Normals.push_back(Normal);
            const FVector Axis{std::abs(Normal.z) < 0.99f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}};
            FVector Tangent{Axis.Cross(Normal).Normalized()};
            float Handedness{1.0f};

            if (SourceMesh.HasTangentsAndBitangents()) {
                const FVector SourceTangent{SourceMesh.mTangents[VertexIndex].x, SourceMesh.mTangents[VertexIndex].y, SourceMesh.mTangents[VertexIndex].z};
                const FVector Bitangent{SourceMesh.mBitangents[VertexIndex].x, SourceMesh.mBitangents[VertexIndex].y, SourceMesh.mBitangents[VertexIndex].z};
                FVector OrthogonalTangent{SourceTangent - Normal * Normal.Dot(SourceTangent)};

                if (OrthogonalTangent.Normalize()) {
                    Tangent = OrthogonalTangent;
                    Handedness = Normal.Cross(Tangent).Dot(Bitangent) < 0.0f ? -1.0f : 1.0f;
                }
            }

            Tangents.push_back({Tangent, Handedness});

            if (SourceMesh.HasVertexColors(0)) {
                const aiColor4D& Color{SourceMesh.mColors[0][VertexIndex]};
                Colors.push_back({Color.r, Color.g, Color.b, Color.a});
            } else {
                Colors.push_back({1.0f, 1.0f, 1.0f, 1.0f});
            }

            for (Uint32 Channel{}; Channel < UVChannelCount; ++Channel) {
                const FVector2 UV{SourceMesh.HasTextureCoords(Channel) ? FVector2{SourceMesh.mTextureCoords[Channel][VertexIndex].x, SourceMesh.mTextureCoords[Channel][VertexIndex].y} : FVector2{}};
                UVChannels[Channel].push_back(UV);
            }
        }

        for (Uint32 FaceIndex{}; FaceIndex < SourceMesh.mNumFaces; ++FaceIndex) {
            const aiFace& Face{SourceMesh.mFaces[FaceIndex]};

            if (Face.mNumIndices != 3) {
                return false;
            }

            for (Uint32 Corner{}; Corner < 3; ++Corner) {
                if (Face.mIndices[Corner] >= SourceMesh.mNumVertices) {
                    return false;
                }

                Indices.push_back(BaseVertex + Face.mIndices[Corner]);
            }
        }
        MeshData.GetSubMeshes().emplace_back(FirstIndex, static_cast<Uint32>(SourceMesh.mNumFaces * 3), SourceMesh.mMaterialIndex);
    }

    return SetMeshData(std::move(MeshData));
}

bool UStaticMesh::SetMeshData(FMeshData MeshData) {
    const std::size_t VertexCount{MeshData.GetPositions().size()};
    const std::size_t ChannelCount{std::max<std::size_t>(1, MeshData.GetUVChannels().size())};
    constexpr std::size_t MaximumBytes{std::numeric_limits<Uint32>::max()};

    if (VertexCount == 0 || MeshData.GetIndices().empty() || MeshData.GetIndices().size() % 3 != 0 || VertexCount > MaximumBytes / GetTangentStride() || MeshData.GetIndices().size() > MaximumBytes / sizeof(Uint32) || ChannelCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount > MaximumBytes / (ChannelCount * sizeof(float) * 2)) {
        return false;
    }

    if ((!MeshData.GetNormals().empty() && MeshData.GetNormals().size() != VertexCount) || (!MeshData.GetTangents().empty() && MeshData.GetTangents().size() != VertexCount) || (!MeshData.GetColors().empty() && MeshData.GetColors().size() != VertexCount)) {
        return false;
    }

    for (Uint32 Index : MeshData.GetIndices()) {
        if (Index >= VertexCount) {
            return false;
        }
    }

    for (const FVector& Normal : MeshData.GetNormals()) {
        if (!Normal.IsFinite()) {
            return false;
        }
    }

    for (const FVector4& Tangent : MeshData.GetTangents()) {
        if (!Tangent.IsFinite()) {
            return false;
        }
    }

    for (const FVector4& Color : MeshData.GetColors()) {
        if (!Color.IsFinite()) {
            return false;
        }
    }

    for (const TArray<FVector2>& Channel : MeshData.GetUVChannels()) {
        if (Channel.size() != VertexCount) {
            return false;
        }

        for (const FVector2& UV : Channel) {
            if (!UV.IsFinite()) {
                return false;
            }
        }
    }

    FVector Minimum{MeshData.GetPositions().front()};
    FVector Maximum{Minimum};

    for (const FVector& Position : MeshData.GetPositions()) {
        if (!Position.IsFinite()) {
            return false;
        }

        Minimum = Minimum.Min(Position);
        Maximum = Maximum.Max(Position);
    }

    if (MeshData.GetMaterialSlots().empty()) {
        MeshData.GetMaterialSlots().emplace_back("Default", FString{});
    }
    if (MeshData.GetSubMeshes().empty()) {
        MeshData.GetSubMeshes().emplace_back(0, static_cast<Uint32>(MeshData.GetIndices().size()), 0);
    }
    Uint32 NextIndex{};
    for (const FSubMesh& SubMesh : MeshData.GetSubMeshes()) {
        if (SubMesh.GetFirstIndex() != NextIndex || SubMesh.GetIndexCount() == 0 || SubMesh.GetIndexCount() % 3 != 0 || SubMesh.GetIndexCount() > MeshData.GetIndices().size() - NextIndex || SubMesh.GetMaterialIndex() >= MeshData.GetMaterialSlots().size()) {
            return false;
        }
        NextIndex += SubMesh.GetIndexCount();
    }
    if (NextIndex != MeshData.GetIndices().size()) {
        return false;
    }
    for (const FMeshMaterialSlot& Slot : MeshData.GetMaterialSlots()) {
        const std::filesystem::path Path{FilePath::FromUtf8(Slot.GetMaterialPath())};
        if (Slot.GetMaterialPath().find('\0') != FString::npos || (!Path.empty() && (Path.has_root_path() || Path.extension() != L".mtl"))) {
            return false;
        }
        for (const auto& Part : Path) {
            if (Part == L"..") {
                return false;
            }
        }
    }
    MeshData.SetBounds({Minimum, Maximum});
    mMeshData = std::move(MeshData);
    ReleaseResources();
    return true;
}

const FMeshData& UStaticMesh::GetMeshData() const {
    return mMeshData;
}

const FBox& UStaticMesh::GetBounds() const {
    return mMeshData.GetBounds();
}

Uint32 UStaticMesh::GetVertexCount() const {
    return static_cast<Uint32>(mMeshData.GetPositions().size());
}

Uint32 UStaticMesh::GetIndexCount() const {
    return static_cast<Uint32>(mMeshData.GetIndices().size());
}

Uint32 UStaticMesh::GetUVChannelCount() const {
    return static_cast<Uint32>(mMeshData.GetUVChannels().size());
}

bool UStaticMesh::IsValid() const {
    return !mMeshData.GetPositions().empty() && !mMeshData.GetIndices().empty();
}

bool UStaticMesh::CreateResources(ID3D11Device* Device) {
    if (Device == nullptr || !IsValid()) {
        return false;
    }

    if (HasResources()) {
        ComPtr<ID3D11Device> ResourceDevice{};
        mPositionBuffer->GetDevice(ResourceDevice.GetAddressOf());

        if (ResourceDevice.Get() == Device) {
            return true;
        }
    }

    const std::size_t VertexCount{mMeshData.GetPositions().size()};
    const std::size_t ChannelCount{std::max<std::size_t>(1, GetUVChannelCount())};
    TArray<std::array<float, 3>> Positions{};
    TArray<std::array<float, 7>> Tangents{};
    TArray<std::array<float, 2>> UVs{};
    TArray<std::array<float, 4>> Colors{};
    Positions.reserve(VertexCount);
    Tangents.reserve(VertexCount);
    UVs.reserve(VertexCount * ChannelCount);
    Colors.reserve(VertexCount);

    for (std::size_t Index{}; Index < VertexCount; ++Index) {
        Positions.push_back(mMeshData.GetPositions()[Index].ToArray());
        const FVector Normal{mMeshData.GetNormals().empty() ? FVector{0.0f, 0.0f, 1.0f} : mMeshData.GetNormals()[Index]};
        const FVector Axis{std::abs(Normal.z) < 0.99f ? FVector{0.0f, 0.0f, 1.0f} : FVector{0.0f, 1.0f, 0.0f}};
        const FVector DefaultTangent{Axis.Cross(Normal).Normalized()};
        const FVector4 Tangent{mMeshData.GetTangents().empty() ? FVector4{DefaultTangent, 1.0f} : mMeshData.GetTangents()[Index]};
        Tangents.push_back({Normal.x, Normal.y, Normal.z, Tangent.x, Tangent.y, Tangent.z, Tangent.w});
        const FVector4 Color{mMeshData.GetColors().empty() ? FVector4{1.0f, 1.0f, 1.0f, 1.0f} : mMeshData.GetColors()[Index]};
        Colors.push_back({Color.x, Color.y, Color.z, Color.w});

        for (std::size_t Channel{}; Channel < ChannelCount; ++Channel) {
            const FVector2 UV{GetUVChannelCount() == 0 ? FVector2{} : mMeshData.GetUVChannels()[Channel][Index]};
            UVs.push_back({UV.x, UV.y});
        }
    }

    std::array<ComPtr<ID3D11Buffer>, 5> Buffers{};

    if (!CreateBuffer(Device, Positions.data(), Positions.size() * GetPositionStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[0]) || !CreateBuffer(Device, Tangents.data(), Tangents.size() * GetTangentStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[1]) || !CreateBuffer(Device, UVs.data(), UVs.size() * sizeof(std::array<float, 2>), D3D11_BIND_VERTEX_BUFFER, Buffers[2]) || !CreateBuffer(Device, Colors.data(), Colors.size() * GetColorStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[3]) || !CreateBuffer(Device, mMeshData.GetIndices().data(), mMeshData.GetIndices().size() * sizeof(Uint32), D3D11_BIND_INDEX_BUFFER, Buffers[4])) {
        return false;
    }

    mPositionBuffer = std::move(Buffers[0]);
    mTangentBuffer = std::move(Buffers[1]);
    mUVBuffer = std::move(Buffers[2]);
    mColorBuffer = std::move(Buffers[3]);
    mIndexBuffer = std::move(Buffers[4]);
    mResourceBytes = VertexCount * (static_cast<std::size_t>(GetPositionStride()) + GetTangentStride() + GetUVStride() + GetColorStride()) + mMeshData.GetIndices().size() * sizeof(Uint32);
    return true;
}

void UStaticMesh::ReleaseResources() {
    mPositionBuffer.Reset();
    mTangentBuffer.Reset();
    mUVBuffer.Reset();
    mColorBuffer.Reset();
    mIndexBuffer.Reset();
    mResourceBytes = 0;
}

bool UStaticMesh::HasResources() const {
    return mPositionBuffer != nullptr && mTangentBuffer != nullptr && mUVBuffer != nullptr && mColorBuffer != nullptr && mIndexBuffer != nullptr;
}

ID3D11Buffer* UStaticMesh::GetPositionBuffer() const {
    return mPositionBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetTangentBuffer() const {
    return mTangentBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetUVBuffer() const {
    return mUVBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetColorBuffer() const {
    return mColorBuffer.Get();
}

ID3D11Buffer* UStaticMesh::GetIndexBuffer() const {
    return mIndexBuffer.Get();
}

Uint32 UStaticMesh::GetUVStride() const {
    return static_cast<Uint32>(std::max<Uint32>(1, GetUVChannelCount()) * sizeof(float) * 2);
}

std::size_t UStaticMesh::GetResourceBytes() const {
    return mResourceBytes;
}

Uint32 UStaticMesh::GetPositionStride() {
    return sizeof(std::array<float, 3>);
}

Uint32 UStaticMesh::GetTangentStride() {
    return sizeof(std::array<float, 7>);
}

Uint32 UStaticMesh::GetColorStride() {
    return sizeof(std::array<float, 4>);
}

bool UStaticMesh::CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t Bytes, Uint32 BindFlags, ComPtr<ID3D11Buffer>& Buffer) {
    if (Device == nullptr || Data == nullptr || Bytes == 0 || Bytes > std::numeric_limits<UINT>::max()) {
        return false;
    }

    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = static_cast<UINT>(Bytes);
    Description.Usage = D3D11_USAGE_IMMUTABLE;
    Description.BindFlags = BindFlags;
    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = Data;
    return SUCCEEDED(Device->CreateBuffer(&Description, &InitialData, Buffer.GetAddressOf()));
}
