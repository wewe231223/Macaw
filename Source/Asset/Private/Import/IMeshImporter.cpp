/**
 * @file IMeshImporter.cpp
 * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
 * @ingroup Asset
 */

#include "Asset/Import/IMeshImporter.h"
#include "Asset/Import/FAssimpMeshImporter.h"
#include "Asset/Import/FFBXImporter.h"
#include "Asset/Import/FMeshImportResult.h"
#include "Asset/FMeshData.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <array>
#include <d3d11.h>
#include <limits>
#include <utility>

namespace Asset {

    /**
     * @brief 가져올 원본 파일 경로와 임포트 옵션을 보관하도록 임포터를 초기화합니다.
     */
    Asset::IMeshImporter::IMeshImporter(std::filesystem::path Source, const Asset::FAssetImportOption& Option)
		: mSourcePath{std::move(Source)}, mOption{Option} {
    }

    Asset::IMeshImporter::~IMeshImporter() = default;

    /**
     * @brief 원본 파일 확장자에 맞는 메시 임포터를 생성합니다.
     */
    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateForSource(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        const FImporterFactory Factory{FindFactory(Source)};

        return Factory != nullptr ? Factory(Source, Option) : CreateAssimp(Source, Option);
    }

    /**
     * @brief 원본 파일 확장자에 등록된 메시 임포터가 있는지 확인합니다.
     */
    bool Asset::IMeshImporter::IsSupported(const std::filesystem::path& Source) {
        return FindFactory(Source) != nullptr;
    }

    /**
     * @brief 메시 형상과 재질 슬롯을 검증하고 기본 슬롯·서브메시 및 경계 상자를 구성합니다.
     */
    bool Asset::IMeshImporter::ValidateMeshData(Asset::FMeshData& MeshData) {
        // 데이터 유입 경로와 관계없이 공유할 형상 규칙과 GPU 버퍼 크기 한도를 검사합니다.
        const std::size_t VertexCount{MeshData.GetPositions().size()};
        const std::size_t ChannelCount{std::max<std::size_t>(1, MeshData.GetUVChannels().size())};
        constexpr std::size_t MaximumBytes{std::numeric_limits<Core::Uint32>::max()};

        if (VertexCount == 0 || MeshData.GetIndices().empty() || MeshData.GetIndices().size() % 3 != 0 || VertexCount > MaximumBytes / (sizeof(float) * 7) || MeshData.GetIndices().size() > MaximumBytes / sizeof(Core::Uint32) || ChannelCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount > MaximumBytes / (ChannelCount * sizeof(float) * 2)) {
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

        // 기본 구성과 경계 계산까지 한곳에서 처리하여 호출자가 다시 순회하지 않게 합니다.
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

        return true;
    }

    /**
     * @brief 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집하고 스킨 가중치 수를 정리합니다.
     */
    bool Asset::IMeshImporter::Import(Asset::FMeshImportResult& Result, Core::FString& Error) {
        Error.clear();

        if (mImportStarted) {
            Error = "A mesh importer can only be used once";

            return false;
        }

        mImportStarted = true;

        if (mSourcePath.empty()) {
            Error = "A mesh source path is required";

            return false;
        }

        std::error_code Code{};

        mSourcePath = std::filesystem::absolute(mSourcePath, Code).lexically_normal();

        if (Code) {
            Error = "Cannot resolve the mesh source path: " + Code.message();

            return false;
        }

        mSourceDirectory = mSourcePath.parent_path();

        if (!mOption.IsValid()) {
            Error = "The import transform options are invalid";

            return false;
        }

        if (!ImportSource()) {
            Error = std::move(mError);

            return false;
        }

        auto& Meshes{mResult.GetMeshes()};
        std::erase_if(Meshes, [](const Asset::FMeshImportMesh& Mesh) {
            return Mesh.GetMeshData().GetPositions().empty();
        });

        if (Meshes.empty()) {
            Error = "The source contains no static mesh triangles";

            return false;
        }

        // 형상 검증과 기본 구성 및 경계 계산은 SetMeshData에서 데이터를 확정할 때 한 번 수행합니다.
        for (Asset::FMeshImportMesh& Mesh : Meshes) {
            Mesh.FinalizeSkinWeights();
        }

        Result = std::move(mResult);

        return true;
    }

    /**
     * @brief 가져오는 원본 파일 경로를 조회합니다.
     */
    const std::filesystem::path& Asset::IMeshImporter::GetSourcePath() const {
        return mSourcePath;
    }

    /**
     * @brief 원본 파일이 위치한 디렉터리를 조회합니다.
     */
    const std::filesystem::path& Asset::IMeshImporter::GetSourceDirectory() const {
        return mSourceDirectory;
    }

    /**
     * @brief 임포터가 따르는 임포트 옵션을 조회합니다.
     */
    const Asset::FAssetImportOption& Asset::IMeshImporter::GetOption() const {
        return mOption;
    }

    /**
     * @brief 파생 임포터가 채울 임포트 결과를 조회합니다.
     */
    Asset::FMeshImportResult& Asset::IMeshImporter::GetResult() {
        return mResult;
    }

    /**
     * @brief 파생 임포터가 실패 원인을 기록할 문자열을 조회합니다.
     */
    Core::FString& Asset::IMeshImporter::GetError() {
        return mError;
    }

    /**
     * @brief 원본 파일 확장자에 등록된 임포터 팩토리를 찾습니다.
     */
    Asset::IMeshImporter::FImporterFactory Asset::IMeshImporter::FindFactory(const std::filesystem::path& Source) {
        static constexpr std::array<FImporterEntry, 4> Importers{FImporterEntry{L".obj", &CreateAssimp}, FImporterEntry{L".fbx", &CreateFBX}, FImporterEntry{L".gltf", &CreateAssimp}, FImporterEntry{L".glb", &CreateAssimp}};
        const std::wstring Extension{Core::FilePath::ToLowerExtension(Source)};

        for (const FImporterEntry& Entry : Importers) {
            if (Entry.first == Extension) {
                return Entry.second;
            }
        }

        return nullptr;
    }

    /**
     * @brief Assimp 기반 메시 임포터를 생성합니다.
     */
    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateAssimp(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        return std::make_unique<Asset::FAssimpMeshImporter>(Source, Option);
    }

    /**
     * @brief FBX SDK 기반 메시 임포터를 생성합니다.
     */
    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateFBX(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        return std::make_unique<Asset::FFBXImporter>(Source, Option);
    }

}
