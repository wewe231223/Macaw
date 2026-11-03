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
            auto Importer{Asset::IMeshImporter::CreateForSource(MeshPath, Asset::FAssetImportOption{})};

            if (!Importer->Import(Imported, Error)) {
                return false;
            }
        }

        // 기본 옵션은 메시를 병합하므로 결과 메시는 항상 하나입니다.
        if (!SetMeshData(std::move(Imported.GetMeshes().front().GetMeshData()))) {
            Error = "The source contains invalid vertex attributes or mesh sections";
            return false;
        }

        return true;
    }

    bool Asset::UStaticMesh::SetMeshData(Asset::FMeshData MeshData) {
        // 파일 임포트 외에 에셋 로드와 직접 생성한 메시에도 동일한 데이터 규칙을 적용합니다.
        if (!Asset::IMeshImporter::ValidateMeshData(MeshData)) {
            return false;
        }

        mMeshData = std::move(MeshData);
        mRevision = Core::FRevision::Next();
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

        if (HasResources() && mPositionBuffer.IsCreatedOn(Device)) {
            return true;
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

        // 다섯 버퍼를 모두 만든 뒤에 교체하여 실패해도 기존 자원을 유지합니다.
        TMeshVertexBuffer<std::array<float, 3>> PositionBuffer{};
        TMeshVertexBuffer<std::array<float, 7>> TangentBuffer{};
        TMeshVertexBuffer<std::array<float, 2>> UVBuffer{};
        TMeshVertexBuffer<std::array<float, 4>> ColorBuffer{};
        RenderCore::TIndexBuffer<Core::Uint32> IndexBuffer{};
        Core::FString Error{};

        if (!PositionBuffer.Create(Device, Positions, Error) || !TangentBuffer.Create(Device, Tangents, Error) || !UVBuffer.Create(Device, UVs, Error) || !ColorBuffer.Create(Device, Colors, Error) || !IndexBuffer.Create(Device, mMeshData.GetIndices(), Error)) {
            return false;
        }

        mPositionBuffer = std::move(PositionBuffer);
        mTangentBuffer = std::move(TangentBuffer);
        mUVBuffer = std::move(UVBuffer);
        mColorBuffer = std::move(ColorBuffer);
        mIndexBuffer = std::move(IndexBuffer);

        return true;
    }

    void Asset::UStaticMesh::ReleaseResources() {
        mPositionBuffer.Reset();
        mTangentBuffer.Reset();
        mUVBuffer.Reset();
        mColorBuffer.Reset();
        mIndexBuffer.Reset();
    }

    bool Asset::UStaticMesh::HasResources() const {
        return mPositionBuffer.IsCreated() && mTangentBuffer.IsCreated() && mUVBuffer.IsCreated() && mColorBuffer.IsCreated() && mIndexBuffer.IsCreated();
    }

    Core::FRevision Asset::UStaticMesh::GetRevision() const {
        return mRevision;
    }

    void Asset::UStaticMesh::Bind(ID3D11DeviceContext* Context) const {
        mPositionBuffer.Bind(Context, 0);
        mTangentBuffer.Bind(Context, 1);
        mUVBuffer.Bind(Context, 2, std::max<Core::Uint32>(1, GetUVChannelCount()));
        mColorBuffer.Bind(Context, 3);
        mIndexBuffer.Bind(Context);
    }

    void Asset::UStaticMesh::BindPositions(ID3D11DeviceContext* Context) const {
        mPositionBuffer.Bind(Context, 0);
        mIndexBuffer.Bind(Context);
    }

    std::size_t Asset::UStaticMesh::GetResourceBytes() const {
        return mPositionBuffer.GetResourceBytes() + mTangentBuffer.GetResourceBytes() + mUVBuffer.GetResourceBytes() + mColorBuffer.GetResourceBytes() + mIndexBuffer.GetResourceBytes();
    }

}
