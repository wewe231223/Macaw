/**
 * @file IMeshImporter.cpp
 * @brief 원본 파일에서 메시 데이터와 재질 참조를 가져오는 임포터의 공통 인터페이스입니다.
 * @ingroup Asset
 */

#include "Asset/Import/IMeshImporter.h"
#include "Asset/Import/FAssimpMeshImporter.h"
#include "Asset/Import/FFBXImporter.h"
#include "Asset/Import/FMeshImportResult.h"
#include "Core/FilePath.h"

#include <array>
#include <utility>

namespace Asset {

    Asset::IMeshImporter::IMeshImporter(std::filesystem::path Source, const Asset::FAssetImportOption& Option)
		: mSourcePath{std::move(Source)}, mOption{Option} {
    }

    Asset::IMeshImporter::~IMeshImporter() = default;

    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateForSource(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        const FImporterFactory Factory{FindFactory(Source)};
        return Factory != nullptr ? Factory(Source, Option) : CreateAssimp(Source, Option);
    }

    bool Asset::IMeshImporter::IsSupported(const std::filesystem::path& Source) {
        return FindFactory(Source) != nullptr;
    }

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

        for (Asset::FMeshImportMesh& Mesh : Meshes) {
            Mesh.GetMeshData().ComputeBounds();
            Mesh.FinalizeSkinWeights();
        }

        Result = std::move(mResult);

        return true;
    }

    const std::filesystem::path& Asset::IMeshImporter::GetSourcePath() const {
        return mSourcePath;
    }

    const std::filesystem::path& Asset::IMeshImporter::GetSourceDirectory() const {
        return mSourceDirectory;
    }

    const Asset::FAssetImportOption& Asset::IMeshImporter::GetOption() const {
        return mOption;
    }

    Asset::FMeshImportResult& Asset::IMeshImporter::GetResult() {
        return mResult;
    }

    Core::FString& Asset::IMeshImporter::GetError() {
        return mError;
    }

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

    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateAssimp(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        return std::make_unique<Asset::FAssimpMeshImporter>(Source, Option);
    }

    std::unique_ptr<Asset::IMeshImporter> Asset::IMeshImporter::CreateFBX(const std::filesystem::path& Source, const Asset::FAssetImportOption& Option) {
        return std::make_unique<Asset::FFBXImporter>(Source, Option);
    }

}
