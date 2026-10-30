/**
 * @file UStaticMesh.cpp
 * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Asset/UStaticMesh.h"
#include "Core/FilePath.h"
#include "Asset/Import/IMeshImporter.h"
#include "Asset/Import/FMeshImportResult.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace Asset {

    Asset::UStaticMesh::UStaticMesh() = default;

    Asset::UStaticMesh::UStaticMesh(const std::filesystem::path& MeshPath) {
        LoadFromFile(MeshPath);
    }

    Asset::UStaticMesh::~UStaticMesh() = default;

    bool Asset::UStaticMesh::LoadFromFile(const std::filesystem::path& MeshPath) {
        Core::FString Error{};
        return LoadFromFile(MeshPath, Error);
    }

    bool Asset::UStaticMesh::LoadFromFile(const std::filesystem::path& MeshPath, Core::FString& Error) {
        Asset::FMeshImportResult Imported{};

        {
            auto Importer{Asset::IMeshImporter::CreateForSource(MeshPath)};

            if (!Importer->Import(Imported, Error)) {
                return false;
            }
        }

        if (!SetMeshData(std::move(Imported.GetMeshData()))) {
            Error = "The source contains invalid vertex attributes or mesh sections";
            return false;
        }

        return true;
    }

    bool Asset::UStaticMesh::SetMeshData(Asset::FMeshData MeshData) {
        const std::size_t VertexCount{MeshData.GetPositions().size()};
        const std::size_t ChannelCount{std::max<std::size_t>(1, MeshData.GetUVChannels().size())};
        constexpr std::size_t MaximumBytes{std::numeric_limits<Core::Uint32>::max()};

        if (VertexCount == 0 || MeshData.GetIndices().empty() || MeshData.GetIndices().size() % 3 != 0 || VertexCount > MaximumBytes / GetTangentStride() || MeshData.GetIndices().size() > MaximumBytes / sizeof(Core::Uint32) || ChannelCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount > MaximumBytes / (ChannelCount * sizeof(float) * 2)) {
            return false;
        }

        if ((!MeshData.GetNormals().empty() && MeshData.GetNormals().size() != VertexCount) || (!MeshData.GetTangents().empty() && MeshData.GetTangents().size() != VertexCount) || (!MeshData.GetColors().empty() && MeshData.GetColors().size() != VertexCount)) {
            return false;
        }

        for (Core::Uint32 Index : MeshData.GetIndices()) {
            if (Index >= VertexCount) {
                return false;
            }
        }

        for (const Core::FVector& Normal : MeshData.GetNormals()) {
            if (!Normal.IsFinite()) {
                return false;
            }
        }

        for (const Core::FVector4& Tangent : MeshData.GetTangents()) {
            if (!Tangent.IsFinite()) {
                return false;
            }
        }

        for (const Core::FVector4& Color : MeshData.GetColors()) {
            if (!Color.IsFinite()) {
                return false;
            }
        }

        for (const Core::TArray<Core::FVector2>& Channel : MeshData.GetUVChannels()) {
            if (Channel.size() != VertexCount) {
                return false;
            }

            for (const Core::FVector2& UV : Channel) {
                if (!UV.IsFinite()) {
                    return false;
                }
            }
        }

        Core::FVector Minimum{MeshData.GetPositions().front()};
        Core::FVector Maximum{Minimum};

        for (const Core::FVector& Position : MeshData.GetPositions()) {
            if (!Position.IsFinite()) {
                return false;
            }

            Minimum = Minimum.Min(Position);
            Maximum = Maximum.Max(Position);
        }

        if (MeshData.GetMaterialSlots().empty()) {
            MeshData.GetMaterialSlots().emplace_back("Default", Core::FString{});
        }

        if (MeshData.GetSubMeshes().empty()) {
            MeshData.GetSubMeshes().emplace_back(0, static_cast<Core::Uint32>(MeshData.GetIndices().size()), 0);
        }

        Core::Uint32 NextIndex{};

        for (const Asset::FSubMesh& SubMesh : MeshData.GetSubMeshes()) {
            if (SubMesh.GetFirstIndex() != NextIndex || SubMesh.GetIndexCount() == 0 || SubMesh.GetIndexCount() % 3 != 0 || SubMesh.GetIndexCount() > MeshData.GetIndices().size() - NextIndex || SubMesh.GetMaterialIndex() >= MeshData.GetMaterialSlots().size()) {
                return false;
            }

            NextIndex += SubMesh.GetIndexCount();
        }

        if (NextIndex != MeshData.GetIndices().size()) {
            return false;
        }

        for (const Asset::FMeshMaterialSlot& Slot : MeshData.GetMaterialSlots()) {
            const std::filesystem::path Path{Core::FilePath::FromUtf8(Slot.GetMaterialPath())};

            if (Slot.GetMaterialPath().find('\0') != Core::FString::npos || (!Path.empty() && (Path.has_root_path() || Path.extension() != L".masset"))) {
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

    const Asset::FMeshData& Asset::UStaticMesh::GetMeshData() const {
        return mMeshData;
    }

    const Core::FBox& Asset::UStaticMesh::GetBounds() const {
        return mMeshData.GetBounds();
    }

    Core::Uint32 Asset::UStaticMesh::GetVertexCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetPositions().size());
    }

    Core::Uint32 Asset::UStaticMesh::GetIndexCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetIndices().size());
    }

    Core::Uint32 Asset::UStaticMesh::GetUVChannelCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetUVChannels().size());
    }

    bool Asset::UStaticMesh::IsValid() const {
        return !mMeshData.GetPositions().empty() && !mMeshData.GetIndices().empty();
    }

    bool Asset::UStaticMesh::CreateResources(ID3D11Device* Device) {
        if (Device == nullptr || !IsValid()) {
            return false;
        }

        if (HasResources()) {
            Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};
            mPositionBuffer->GetDevice(ResourceDevice.GetAddressOf());

            if (ResourceDevice.Get() == Device) {
                return true;
            }
        }

        const std::size_t VertexCount{mMeshData.GetPositions().size()};
        const std::size_t ChannelCount{std::max<std::size_t>(1, GetUVChannelCount())};
        Core::TArray<std::array<float, 3>> Positions{};
        Core::TArray<std::array<float, 7>> Tangents{};
        Core::TArray<std::array<float, 2>> UVs{};
        Core::TArray<std::array<float, 4>> Colors{};
        Positions.reserve(VertexCount);
        Tangents.reserve(VertexCount);
        UVs.reserve(VertexCount * ChannelCount);
        Colors.reserve(VertexCount);

        for (std::size_t Index{}; Index < VertexCount; ++Index) {
            Positions.push_back(mMeshData.GetPositions()[Index].ToArray());

            const Core::FVector Normal{mMeshData.GetNormals().empty() ? Core::FVector{0.0f, 0.0f, 1.0f} : mMeshData.GetNormals()[Index]};
            const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
            const Core::FVector DefaultTangent{Axis.Cross(Normal).Normalized()};
            const Core::FVector4 Tangent{mMeshData.GetTangents().empty() ? Core::FVector4{DefaultTangent, 1.0f} : mMeshData.GetTangents()[Index]};
            Tangents.push_back({Normal.x, Normal.y, Normal.z, Tangent.x, Tangent.y, Tangent.z, Tangent.w});

            const Core::FVector4 Color{mMeshData.GetColors().empty() ? Core::FVector4{1.0f, 1.0f, 1.0f, 1.0f} : mMeshData.GetColors()[Index]};
            Colors.push_back({Color.x, Color.y, Color.z, Color.w});

            for (std::size_t Channel{}; Channel < ChannelCount; ++Channel) {
                const Core::FVector2 UV{GetUVChannelCount() == 0 ? Core::FVector2{} : mMeshData.GetUVChannels()[Channel][Index]};
                UVs.push_back({UV.x, UV.y});
            }
        }

        std::array<Microsoft::WRL::ComPtr<ID3D11Buffer>, 5> Buffers{};

        if (!CreateBuffer(Device, Positions.data(), Positions.size() * GetPositionStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[0]) || !CreateBuffer(Device, Tangents.data(), Tangents.size() * GetTangentStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[1]) || !CreateBuffer(Device, UVs.data(), UVs.size() * sizeof(std::array<float, 2>), D3D11_BIND_VERTEX_BUFFER, Buffers[2]) || !CreateBuffer(Device, Colors.data(), Colors.size() * GetColorStride(), D3D11_BIND_VERTEX_BUFFER, Buffers[3]) || !CreateBuffer(Device, mMeshData.GetIndices().data(), mMeshData.GetIndices().size() * sizeof(Core::Uint32), D3D11_BIND_INDEX_BUFFER, Buffers[4])) {
            return false;
        }

        mPositionBuffer = std::move(Buffers[0]);
        mTangentBuffer = std::move(Buffers[1]);
        mUVBuffer = std::move(Buffers[2]);
        mColorBuffer = std::move(Buffers[3]);
        mIndexBuffer = std::move(Buffers[4]);
        mResourceBytes = VertexCount * (static_cast<std::size_t>(GetPositionStride()) + GetTangentStride() + GetUVStride() + GetColorStride()) + mMeshData.GetIndices().size() * sizeof(Core::Uint32);

        return true;
    }

    void Asset::UStaticMesh::ReleaseResources() {
        mPositionBuffer.Reset();
        mTangentBuffer.Reset();
        mUVBuffer.Reset();
        mColorBuffer.Reset();
        mIndexBuffer.Reset();
        mResourceBytes = 0;
    }

    bool Asset::UStaticMesh::HasResources() const {
        return mPositionBuffer != nullptr && mTangentBuffer != nullptr && mUVBuffer != nullptr && mColorBuffer != nullptr && mIndexBuffer != nullptr;
    }

    ID3D11Buffer* Asset::UStaticMesh::GetPositionBuffer() const {
        return mPositionBuffer.Get();
    }

    ID3D11Buffer* Asset::UStaticMesh::GetTangentBuffer() const {
        return mTangentBuffer.Get();
    }

    ID3D11Buffer* Asset::UStaticMesh::GetUVBuffer() const {
        return mUVBuffer.Get();
    }

    ID3D11Buffer* Asset::UStaticMesh::GetColorBuffer() const {
        return mColorBuffer.Get();
    }

    ID3D11Buffer* Asset::UStaticMesh::GetIndexBuffer() const {
        return mIndexBuffer.Get();
    }

    Core::Uint32 Asset::UStaticMesh::GetUVStride() const {
        return static_cast<Core::Uint32>(std::max<Core::Uint32>(1, GetUVChannelCount()) * sizeof(float) * 2);
    }

    std::size_t Asset::UStaticMesh::GetResourceBytes() const {
        return mResourceBytes;
    }

    Core::Uint32 Asset::UStaticMesh::GetPositionStride() {
        return sizeof(std::array<float, 3>);
    }

    Core::Uint32 Asset::UStaticMesh::GetTangentStride() {
        return sizeof(std::array<float, 7>);
    }

    Core::Uint32 Asset::UStaticMesh::GetColorStride() {
        return sizeof(std::array<float, 4>);
    }

    bool Asset::UStaticMesh::CreateBuffer(ID3D11Device* Device, const void* Data, std::size_t Bytes, Core::Uint32 BindFlags, Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer) {
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

}
