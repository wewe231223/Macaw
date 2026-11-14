/**
 * @file UStaticMesh.cpp
 * @brief CPU 메시 데이터와 Direct3D 정점·인덱스 버퍼 관리에 대한 구현입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
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

    /**
     * @brief UStaticMesh의 초기 상태를 구성합니다.
     */
    Asset::UStaticMesh::UStaticMesh() = default;

    /**
     * @brief 전달받은 설정과 의존성으로 UStaticMesh의 초기 상태를 구성합니다.
     */
    Asset::UStaticMesh::UStaticMesh(const std::filesystem::path& MeshPath) {
        LoadFromFile(MeshPath);
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Asset::UStaticMesh::~UStaticMesh() = default;

    /**
     * @brief 외부 메시 파일을 가져와 정점 속성과 인덱스 및 경계를 구성합니다.
     */
    bool Asset::UStaticMesh::LoadFromFile(const std::filesystem::path& MeshPath) {
        Core::FString Error{};

        return LoadFromFile(MeshPath, Error);
    }

    /**
     * @brief 메시 파일을 읽고 지정된 에셋 관리자로 재질과 텍스처를 해석합니다.
     */
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

    /**
     * @brief CPU 메시 데이터의 유효성과 경계를 검사하여 교체하고 GPU 버퍼를 해제합니다.
     */
    bool Asset::UStaticMesh::SetMeshData(Asset::FMeshData MeshData) {
        // 파일 임포트 외에 에셋 로드와 직접 생성한 메시에도 동일한 데이터 규칙을 적용합니다.
        if (!Asset::IMeshImporter::ValidateMeshData(MeshData)) {
            return false;
        }

        mMeshData = std::move(MeshData);

        ReleaseResources();

        return true;
    }

    /**
     * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
     */
    const Asset::FMeshData& Asset::UStaticMesh::GetMeshData() const {
        return mMeshData;
    }

    /**
     * @brief 메시의 로컬 공간 경계 상자를 반환합니다.
     */
    const Core::FBox& Asset::UStaticMesh::GetBounds() const {
        return mMeshData.GetBounds();
    }

    /**
     * @brief 메시의 정점 개수를 반환합니다.
     */
    Core::Uint32 Asset::UStaticMesh::GetVertexCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetPositions().size());
    }

    /**
     * @brief 메시의 인덱스 개수를 반환합니다.
     */
    Core::Uint32 Asset::UStaticMesh::GetIndexCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetIndices().size());
    }

    /**
     * @brief 메시에 포함된 UV 채널 수를 반환합니다.
     */
    Core::Uint32 Asset::UStaticMesh::GetUVChannelCount() const {
        return static_cast<Core::Uint32>(mMeshData.GetUVChannels().size());
    }

    /**
     * @brief CPU 메시의 정점·인덱스·속성 배열이 렌더링 조건을 만족하는지 검사합니다.
     */
    bool Asset::UStaticMesh::IsValid() const {
        return !mMeshData.GetPositions().empty() && !mMeshData.GetIndices().empty();
    }

    /**
     * @brief 현재 CPU 데이터를 사용하여 Direct3D 리소스를 생성합니다.
     */
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

        for (Core::Index_t Index{}; Index < VertexCount; ++Index) {
            Positions.push_back(mMeshData.GetPositions()[Index].ToArray());

            const Core::FVector Normal{mMeshData.GetNormals().empty() ? Core::FVector{0.0f, 0.0f, 1.0f} : mMeshData.GetNormals()[Index]};
            const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
            const Core::FVector DefaultTangent{Axis.Cross(Normal).Normalized()};
            const Core::FVector4 Tangent{mMeshData.GetTangents().empty() ? Core::FVector4{DefaultTangent, 1.0f} : mMeshData.GetTangents()[Index]};

            Tangents.push_back({Normal.x, Normal.y, Normal.z, Tangent.x, Tangent.y, Tangent.z, Tangent.w});

            const Core::FVector4 Color{mMeshData.GetColors().empty() ? Core::FVector4{1.0f, 1.0f, 1.0f, 1.0f} : mMeshData.GetColors()[Index]};

            Colors.push_back({Color.x, Color.y, Color.z, Color.w});

            for (Core::Index_t Channel{}; Channel < ChannelCount; ++Channel) {
                const Core::FVector2 UV{GetUVChannelCount() == 0 ? Core::FVector2{} : mMeshData.GetUVChannels()[Channel][Index]};

                UVs.push_back({UV.x, UV.y});
            }
        }

        // 다섯 버퍼를 모두 만든 뒤에 교체하여 실패해도 기존 자원을 유지합니다.
        TMeshVertexBuffer<std::array<float, 3>> PositionBuffer{};
        TMeshVertexBuffer<std::array<float, 7>> TangentBuffer{};
        TMeshVertexBuffer<std::array<float, 2>> UVBuffer{};
        TMeshVertexBuffer<std::array<float, 4>> ColorBuffer{};
        RenderCore::TIndexBuffer<Core::Index_t> IndexBuffer{};
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

    /**
     * @brief 소유한 GPU 리소스와 뷰를 해제하고 메모리 집계를 초기화합니다.
     */
    void Asset::UStaticMesh::ReleaseResources() {
        mPositionBuffer.Reset();
        mTangentBuffer.Reset();
        mUVBuffer.Reset();
        mColorBuffer.Reset();
        mIndexBuffer.Reset();
    }

    /**
     * @brief 사용할 수 있는 GPU 리소스를 보유하는지 검사합니다.
     */
    bool Asset::UStaticMesh::HasResources() const {
        return mPositionBuffer.IsCreated() && mTangentBuffer.IsCreated() && mUVBuffer.IsCreated() && mColorBuffer.IsCreated() && mIndexBuffer.IsCreated();
    }

    /**
     * @brief 위치·법선·UV·색상 버퍼를 입력 슬롯 0~3에, 인덱스 버퍼를 함께 연결합니다.
     */
    void Asset::UStaticMesh::Bind(ID3D11DeviceContext* Context) const {
        mPositionBuffer.Bind(Context, 0);
        mTangentBuffer.Bind(Context, 1);
        mUVBuffer.Bind(Context, 2, std::max<Core::Uint32>(1, GetUVChannelCount()));
        mColorBuffer.Bind(Context, 3);
        mIndexBuffer.Bind(Context);
    }

    /**
     * @brief 위치 버퍼만 입력 슬롯 0에, 인덱스 버퍼를 함께 연결합니다.
     */
    void Asset::UStaticMesh::BindPositions(ID3D11DeviceContext* Context) const {
        mPositionBuffer.Bind(Context, 0);
        mIndexBuffer.Bind(Context);
    }

    /**
     * @brief 현재 보유한 GPU 리소스의 추정 바이트 수를 반환합니다.
     */
    std::size_t Asset::UStaticMesh::GetResourceBytes() const {
        return mPositionBuffer.GetResourceBytes() + mTangentBuffer.GetResourceBytes() + mUVBuffer.GetResourceBytes() + mColorBuffer.GetResourceBytes() + mIndexBuffer.GetResourceBytes();
    }

}
