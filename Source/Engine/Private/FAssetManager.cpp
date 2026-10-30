/**
 * @file FAssetManager.cpp
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FAssetManager.h"
#include "Engine/EngineStats.h"
#include "Core/FilePath.h"
#include "Engine/FAssetFile.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UTexture2D.h"
#include "Engine/UMaterial.h"
#include "FMaterialFile.h"
#include "FMaterialAssetData.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Serialization/MemoryArchive.h"
#include "FAssetSerialization.h"
#include "FTextureAssetData.h"
#include "FAssetImportScope.h"
#include "Engine/Import/IMeshImporter.h"
#include "Engine/Import/FMeshImportResult.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>

namespace Engine {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 에셋 상대 경로를 대소문자 구분 없이 조회할 캐시 키로 변환합니다.
         * @param[in] Relative 캐시 키로 변환할 프로젝트 상대 에셋 경로입니다.
         * @return 입력 값을 변환한 문자열입니다.
         */
        Core::FString CacheKey(const Core::FString& Relative) {
            std::wstring Key{Core::FilePath::FromUtf8(Relative).wstring()};

            for (wchar_t& Character : Key) {
                Character = static_cast<wchar_t>(std::towlower(Character));
            }

            return Core::FilePath::ToGenericUtf8(std::filesystem::path{Key});
        }

        /**
         * @brief 텍스트 파일에 이전 재질 형식의 shader 지시자가 있는지 검사합니다.
         * @param[in] Path 읽거나 기록할 파일 시스템 경로입니다.
         * @return shader 키워드가 발견되면 true이며 그렇지 않으면 false입니다.
         */
        bool IsLegacyMaterialAsset(const std::filesystem::path& Path) {
            std::ifstream Input{Path};
            Core::FString Line{};

            while (std::getline(Input, Line)) {
                std::istringstream Values{Line};
                Core::FString Keyword{};
                Values >> Keyword;

                if (Keyword == "shader") {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief 파일명 금지 문자·끝 공백과 점·Windows 예약 이름을 정리합니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[in] Fallback 정리한 이름이 비었을 때 사용할 대체 파일 이름입니다.
         * @return 안전하게 정리한 이름이며 입력이 비어 있으면 대체 이름을 사용합니다.
         */
        Core::FString AssetFileName(Core::FString Name, const Core::FString& Fallback) {
            for (char& Character : Name) {
                if (static_cast<unsigned char>(Character) < 32 || Core::FString{"<>:\"/\\|?*"}.find(Character) != Core::FString::npos) {
                    Character = '_';
                }
            }

            while (!Name.empty() && (Name.back() == '.' || Name.back() == ' ')) {
                Name.pop_back();
            }

            if (Name.empty()) {
                Name = Fallback;
            }

            Core::FString Base{Name.substr(0, Name.find('.'))};

            for (char& Character : Base) {
                if (Character >= 'a' && Character <= 'z') {
                    Character = static_cast<char>(Character - 'a' + 'A');
                }
            }

            if (Base == "CON" || Base == "PRN" || Base == "AUX" || Base == "NUL" || (Base.size() == 4 && (Base.starts_with("COM") || Base.starts_with("LPT")) && Base.back() >= '1' && Base.back() <= '9')) {
                Name.insert(Name.begin(), '_');
            }

            return Name;
        }

        /**
         * @brief 기존 파일과 겹치지 않도록 숫자 접미사를 붙인 .masset 경로를 선택합니다.
         * @param[in] Directory 에셋 파일을 만들 폴더입니다.
         * @param[in] Name 조회하거나 지정할 이름입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 선택한 파일 경로이며 파일 시스템 검사 실패 시 빈 경로입니다.
         */
        std::filesystem::path UniqueAssetPath(const std::filesystem::path& Directory, const Core::FString& Name, Core::FString& Error) {
            const auto Base{Core::FilePath::FromUtf8(Name)};
            auto Target{Directory / Base};
            Target += L".masset";
            std::error_code Code{};

            for (Core::Uint32 Index{1}; std::filesystem::exists(Target, Code) && !Code; ++Index) {
                Target = Directory / (Base.wstring() + L"_" + std::to_wstring(Index) + L".masset");
            }

            if (Code) {
                Error = "Cannot choose an asset filename: " + Code.message();
                return {};
            }

            return Target;
        }

        /**
         * @brief 외부 재질 텍스처를 중복 경로 캐시와 공유하여 가져오고 프로젝트 상대 참조를 반환합니다.
         * @param[in,out] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[in] Texture 가져올 원본 텍스처 참조입니다.
         * @param[in] Directory 가져온 텍스처 에셋을 저장할 폴더입니다.
         * @param[in,out] Textures 원본 경로별로 이미 가져온 텍스처의 프로젝트 참조를 보관하는 캐시입니다.
         * @param[out] ImportedPath 가져온 텍스처의 프로젝트 상대 경로를 기록할 문자열입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 텍스처가 없거나 가져오기에 성공하면 true이며 내장 텍스처 또는 파일 오류이면 false입니다.
         */
        bool ImportMeshTexture(Engine::FAssetManager& Assets, const Engine::FMeshImportTexture& Texture, const std::filesystem::path& Directory, Core::TMap<Core::FString, Core::FString>& Textures, Core::FString& ImportedPath, Core::FString& Error) {
            ImportedPath.clear();

            if (Texture.GetSource() == Engine::EMeshImportTextureSource::None) {
                return true;
            }

            if (Texture.GetSource() == Engine::EMeshImportTextureSource::Embedded) {
                Error = "Embedded material textures are not supported: " + Texture.GetEmbeddedReference();
                return false;
            }

            const auto& Source{Texture.GetFilePath()};
            const Core::FString Key{CacheKey(Core::FilePath::ToUtf8(Source))};
            auto Existing{Textures.find(Key)};

            if (Existing == Textures.end()) {
                const auto Target{UniqueAssetPath(Directory, AssetFileName(Core::FilePath::ToUtf8(Source.stem()), "Texture"), Error)};

                if (Target.empty() || !Assets.ImportTexture(Source, Target, Error)) {
                    Error = "Cannot import material texture '" + Core::FilePath::ToUtf8(Source) + "': " + Error;
                    return false;
                }

                const Core::FString RelativePath{Assets.GetRelativePath(Target, Error)};

                if (RelativePath.empty()) {
                    return false;
                }

                Existing = Textures.emplace(Key, RelativePath).first;
            }

            ImportedPath = Existing->second;

            return true;
        }

        /**
         * @brief 메시 정점 속성과 UV 채널 및 인덱스를 바이너리로 직렬화합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in,out] Mesh 읽거나 직렬화할 메시 데이터입니다.
         */
        void SerializeMesh(Serialization::FMemoryArchive& Archive, Engine::FMeshData& Mesh) {
            Engine::FAssetSerialization::SerializeValues(Archive, Mesh.GetPositions());
            Engine::FAssetSerialization::SerializeValues(Archive, Mesh.GetNormals());
            Engine::FAssetSerialization::SerializeValues(Archive, Mesh.GetTangents());
            Engine::FAssetSerialization::SerializeValues(Archive, Mesh.GetColors());

            Core::Uint32 Channels{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetUVChannels().size())};
            Archive.Serialize("UVChannels", Channels);

            if (Archive.HasError() || Channels > 8) {
                Archive.SetError("Invalid mesh UV channel count");
                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetUVChannels().resize(Channels);
            }

            for (auto& Channel : Mesh.GetUVChannels()) {
                Engine::FAssetSerialization::SerializeValues(Archive, Channel);
            }

            Engine::FAssetSerialization::SerializeValues(Archive, Mesh.GetIndices());

            if (Archive.HasError() || (Archive.IsLoading() && Archive.GetRemainingBytes() == 0)) {
                return;
            }

            Core::Uint32 ExtensionVersion{1};
            Archive.Serialize("SectionsVersion", ExtensionVersion);

            if (ExtensionVersion != 1) {
                Archive.SetError("Unsupported mesh section version");
                return;
            }

            Core::Uint32 SlotCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetMaterialSlots().size())};

            if (!Archive.BeginArray("MaterialSlots", SlotCount) || (Archive.IsLoading() && SlotCount > Archive.GetRemainingBytes() / 8)) {
                Archive.SetError("Invalid mesh material slot count");
                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetMaterialSlots().clear();
            }

            for (Core::Uint32 Index{}; Index < SlotCount && !Archive.HasError(); ++Index) {
                Core::FString Name{Archive.IsLoading() ? Core::FString{} : Mesh.GetMaterialSlots()[Index].GetName()};
                Core::FString Path{Archive.IsLoading() ? Core::FString{} : Mesh.GetMaterialSlots()[Index].GetMaterialPath()};
                Archive.Serialize("Name", Name);
                Archive.Serialize("Material", Path);

                if (Archive.IsLoading()) {
                    Mesh.GetMaterialSlots().emplace_back(std::move(Name), std::move(Path));
                }
            }

            Archive.EndArray();

            Core::Uint32 SectionCount{Archive.IsLoading() ? 0 : static_cast<Core::Uint32>(Mesh.GetSubMeshes().size())};

            if (!Archive.BeginArray("SubMeshes", SectionCount) || (Archive.IsLoading() && SectionCount > Archive.GetRemainingBytes() / 12)) {
                Archive.SetError("Invalid submesh count");
                return;
            }

            if (Archive.IsLoading()) {
                Mesh.GetSubMeshes().clear();
            }

            for (Core::Uint32 Index{}; Index < SectionCount && !Archive.HasError(); ++Index) {
                Core::Uint32 First{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetFirstIndex()};
                Core::Uint32 Count{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetIndexCount()};
                Core::Uint32 Slot{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetMaterialIndex()};
                Archive.Serialize("FirstIndex", First);
                Archive.Serialize("IndexCount", Count);
                Archive.Serialize("MaterialIndex", Slot);

                if (Archive.IsLoading()) {
                    Mesh.GetSubMeshes().emplace_back(First, Count, Slot);
                }
            }

            Archive.EndArray();
        }

        /**
         * @brief MTL 참조를 새 재질 에셋 경로로 치환하고 필요한 재질을 임시 디렉터리에 준비합니다.
         * @param[in,out] Reference 장면의 머티리얼 참조이며, 이전하면 새 에셋 경로로 바뀝니다.
         * @param[in,out] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[in] Materials 이전 재질 경로에서 새 재질 에셋 경로로의 대응표입니다.
         * @param[in] Directory 이전한 머티리얼을 임시로 저장할 폴더입니다.
         * @param[in,out] Targets 임시 파일을 최종 위치로 옮길 때 사용할 재질 에셋 경로 목록입니다.
         * @param[in,out] Changed 참조가 치환되면 true로 갱신할 변경 상태입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 변환이 필요 없거나 변환 준비를 완료하면 true이며 충돌·참조 오류이면 false입니다.
         */
        bool MigrateMaterialReference(Core::FString& Reference, Engine::FAssetManager& Assets, const Core::TMap<Core::FString, Core::FString>& Materials, const std::filesystem::path& Directory, Core::TArray<std::filesystem::path>& Targets, bool& Changed, Core::FString& Error) {
            const auto Path{Core::FilePath::FromUtf8(Reference)};

            if (Path.extension() != L".mtl") {
                return true;
            }

            const Core::FString Key{CacheKey(Assets.GetRelativePath(Path, Error))};
            const auto Position{Materials.find(Key)};

            if (Key.empty() || Position == Materials.end()) {
                Error = "Cannot migrate material reference: " + Reference;
                return false;
            }

            const auto Target{Assets.ResolvePath(Core::FilePath::FromUtf8(Position->second), Error)};
            std::error_code Code{};
            const bool Exists{!Target.empty() && std::filesystem::exists(Target, Code)};

            if (Target.empty() || Code) {
                Error = "Cannot inspect material reference migration target: " + Reference;
                return false;
            }

            if (Exists) {
                auto Legacy{CoreUObject::NewObject<Engine::UMaterial>()};
                Core::TArray<Core::Uint8> Actual{};

                if (!Engine::FMaterialFile::LoadLegacy(Assets.ResolvePath(Path, Error), Assets, *Legacy, Error) || !Engine::FAssetFile::Load(Target, Engine::EAssetType::Material, Actual, Error)) {
                    return false;
                }

                Core::TArray<Core::Uint8> Expected{};
                Serialization::FMemoryArchive Writer{Expected};
                Engine::FMaterialAssetData Data{*Legacy};
                Data.Serialize(Writer);

                if (Writer.HasError() || Expected != Actual) {
                    Error = "Material migration target conflicts with the legacy material: " + Position->second;
                    return false;
                }
            } else if (std::find(Targets.begin(), Targets.end(), Target) == Targets.end()) {
                auto Legacy{CoreUObject::NewObject<Engine::UMaterial>()};
                const auto Staged{Directory / (std::to_string(Targets.size()) + ".masset")};

                if (!Engine::FMaterialFile::LoadLegacy(Assets.ResolvePath(Path, Error), Assets, *Legacy, Error) || !Engine::FMaterialFile::Save(Staged, Assets, *Legacy, Error)) {
                    return false;
                }

                Targets.push_back(Target);
            }

            Reference = Position->second;
            Changed = true;

            return true;
        }

        /**
         * @brief 장면 JSON을 깊이 제한 안에서 순회하며 Material·SlotMaterials의 이전 참조를 치환합니다.
         * @param[in,out] Value 재귀 순회하며 재질 참조를 갱신할 JSON 값입니다.
         * @param[in,out] Allocator 문자열 치환에 사용할 원본 JSON 문서의 할당자입니다.
         * @param[in,out] Assets 경로 경계를 확인하고 에셋을 가져올 프로젝트 에셋 관리자입니다.
         * @param[in] Materials 이전 재질 경로에서 새 재질 에셋 경로로의 대응표입니다.
         * @param[in] Directory 이전한 머티리얼을 임시로 저장할 폴더입니다.
         * @param[in,out] Targets 임시 파일을 최종 위치로 옮길 때 사용할 재질 에셋 경로 목록입니다.
         * @param[in,out] Changed 참조가 치환되면 true로 갱신할 변경 상태입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @param[in] MaterialReference 현재 문자열이 재질 경로로 해석될 위치인지 나타냅니다.
         * @param[in] Depth 현재 JSON 순회 깊이이며 32를 넘으면 실패합니다.
         * @return 전체 순회를 완료하면 true이며 깊이 제한 또는 재질 이전 오류이면 false입니다.
         */
        bool MigrateSceneReferences(rapidjson::Value& Value, rapidjson::Document::AllocatorType& Allocator, Engine::FAssetManager& Assets, const Core::TMap<Core::FString, Core::FString>& Materials, const std::filesystem::path& Directory, Core::TArray<std::filesystem::path>& Targets, bool& Changed, Core::FString& Error, bool MaterialReference, Core::Uint32 Depth) {
            if (Depth > 32) {
                Error = "Scene nesting exceeds the migration limit";
                return false;
            }

            if (Value.IsObject()) {
                for (auto Position{Value.MemberBegin()}; Position != Value.MemberEnd(); ++Position) {
                    const std::string_view Name{Position->name.GetString(), Position->name.GetStringLength()};

                    if (!MigrateSceneReferences(Position->value, Allocator, Assets, Materials, Directory, Targets, Changed, Error, Name == "Material" || Name == "SlotMaterials", Depth + 1)) {
                        return false;
                    }
                }
            } else if (Value.IsArray()) {
                for (auto& Child : Value.GetArray()) {
                    if (!MigrateSceneReferences(Child, Allocator, Assets, Materials, Directory, Targets, Changed, Error, MaterialReference, Depth + 1)) {
                        return false;
                    }
                }
            } else if (MaterialReference && Value.IsString()) {
                Core::FString Reference{Value.GetString(), Value.GetStringLength()};
                bool Replaced{};

                if (!MigrateMaterialReference(Reference, Assets, Materials, Directory, Targets, Replaced, Error)) {
                    return false;
                }

                if (Replaced) {
                    Value.SetString(Reference.data(), static_cast<rapidjson::SizeType>(Reference.size()), Allocator);
                    Changed = true;
                }
            }

            return true;
        }
    }

    Engine::FAssetManager::FAssetManager(std::filesystem::path ContentRoot)
		: mContentRoot{std::filesystem::absolute(ContentRoot).lexically_normal()} {
    }

    Engine::FAssetManager::~FAssetManager() = default;

    const std::filesystem::path& Engine::FAssetManager::GetContentRoot() const {
        return mContentRoot;
    }

    Core::FString Engine::FAssetManager::GetMeshPath(const Engine::UStaticMesh* Mesh) const {
        for (const auto& Entry : mMeshes) {
            if (Entry.second.get() == Mesh) {
                return Entry.first;
            }
        }

        return {};
    }

    Core::FString Engine::FAssetManager::GetTexturePath(const Engine::UTexture2D* Texture) const {
        for (const auto& Entry : mTextures) {
            if (Entry.second.get() == Texture) {
                return Entry.first;
            }
        }

        return {};
    }

    std::filesystem::path Engine::FAssetManager::ResolvePath(const std::filesystem::path& Path, Core::FString& Error) const {
        Error.clear();

        if (Path.empty() || (Path.has_root_name() && !Path.is_absolute())) {
            Error = "An asset path is required";
            return {};
        }

        std::error_code Code{};
        const auto Root{std::filesystem::weakly_canonical(mContentRoot, Code)};
        const auto Resolved{Code ? std::filesystem::path{} : std::filesystem::weakly_canonical(Path.is_absolute() ? Path : Root / Path, Code)};

        if (Code || Root.empty() || Resolved.empty()) {
            Error = "Cannot resolve the project asset path";
            return {};
        }

        auto RootPart{Root.begin()};
        auto PathPart{Resolved.begin()};

        for (; RootPart != Root.end() && PathPart != Resolved.end(); ++RootPart, ++PathPart) {
            if (_wcsicmp(RootPart->c_str(), PathPart->c_str()) != 0) {
                break;
            }
        }

        if (RootPart != Root.end() || PathPart == Resolved.end()) {
            Error = "Assets must be inside the current project's Content folder";
            return {};
        }

        return Resolved;
    }

    Core::FString Engine::FAssetManager::GetRelativePath(const std::filesystem::path& Path, Core::FString& Error) const {
        const auto Resolved{ResolvePath(Path, Error)};

        if (Resolved.empty()) {
            return {};
        }

        std::error_code Code{};
        const auto Relative{std::filesystem::relative(Resolved, mContentRoot, Code)};

        if (Code) {
            Error = "Cannot make a project relative asset path";
            return {};
        }

        return Core::FilePath::ToGenericUtf8(Relative);
    }

    Core::TArray<std::filesystem::path> Engine::FAssetManager::GetAssetPaths(Core::FString& Error) const {
        Error.clear();

        Core::TArray<std::filesystem::path> Paths{};
        std::error_code Code{};
        std::filesystem::recursive_directory_iterator Iterator{mContentRoot, Code};
        const std::filesystem::recursive_directory_iterator End{};

        while (!Code && Iterator != End) {
            if (Iterator->is_regular_file(Code) && Iterator->path().extension() == L".masset") {
                const auto Resolved{ResolvePath(Iterator->path(), Error)};

                if (Resolved.empty()) {
                    return {};
                }

                Paths.push_back(Resolved.lexically_relative(mContentRoot));
            }

            Iterator.increment(Code);
        }

        if (Code) {
            Error = "Cannot enumerate project assets: " + Code.message();
        }

        std::sort(Paths.begin(), Paths.end());

        return Paths;
    }

    Engine::UStaticMesh* Engine::FAssetManager::LoadMesh(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::MeshLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mMeshes.find(Key)};

        if (Existing != mMeshes.end()) {
            Engine::EngineStats::AssetCacheHits.Add();
            return Existing->second.get();
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Engine::FAssetFile::Load(ResolvePath(Path, Error), Engine::EAssetType::Mesh, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Engine::FMeshData Data{};
        SerializeMesh(Archive, Data);

        auto Mesh{CoreUObject::NewObject<Engine::UStaticMesh>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || !Mesh->SetMeshData(std::move(Data))) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid mesh asset data";
            return nullptr;
        }

        Engine::UStaticMesh* Result{Mesh.get()};

        Mesh->SetName(Core::FilePath::ToUtf8(Path.stem()));
        mMeshes.emplace(Key, std::move(Mesh));

        return Result;
    }

    Engine::UTexture2D* Engine::FAssetManager::LoadTexture(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::TextureLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mTextures.find(Key)};

        if (Existing != mTextures.end()) {
            Engine::EngineStats::AssetCacheHits.Add();
            return Existing->second.get();
        }

        const auto Source{ResolvePath(Path, Error)};

        if (Source.empty()) {
            return nullptr;
        }

        if (Source.extension() != L".masset") {
            auto Texture{CoreUObject::NewObject<Engine::UTexture2D>()};

            if (!Texture->LoadFromFile(Source)) {
                Error = "Cannot load the material texture";
                return nullptr;
            }

            Engine::UTexture2D* Result{Texture.get()};

            mTextures.emplace(Key, std::move(Texture));

            return Result;
        }

        Core::TArray<Core::Uint8> Payload{};

        if (!Engine::FAssetFile::Load(ResolvePath(Path, Error), Engine::EAssetType::Texture, Payload, Error)) {
            return nullptr;
        }

        Serialization::FMemoryArchive Archive{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
        Engine::FTextureAssetData Data{};
        Data.Serialize(Archive);

        auto Texture{CoreUObject::NewObject<Engine::UTexture2D>()};

        if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
            Error = Archive.HasError() ? Archive.GetError() : "Invalid texture asset data";
            return nullptr;
        }

        if (!Data.ApplyTo(*Texture, Error)) {
            return nullptr;
        }

        Engine::UTexture2D* Result{Texture.get()};

        mTextures.emplace(Key, std::move(Texture));

        return Result;
    }

    bool Engine::FAssetManager::SaveMesh(const std::filesystem::path& Path, const Engine::UStaticMesh& Mesh, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Mesh.IsValid()) {
            if (Error.empty()) {
                Error = "Cannot save an invalid mesh";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Engine::FMeshData Data{Mesh.GetMeshData()};
        SerializeMesh(Archive, Data);

        if (Archive.HasError()) {
            Error = Archive.GetError();
            return false;
        }

        if (!Engine::FAssetFile::Save(Target, Engine::EAssetType::Mesh, Payload, Error)) {
            return false;
        }

        const auto Existing{mMeshes.find(CacheKey(GetRelativePath(Path, Error)))};

        if (Existing != mMeshes.end() && Existing->second.get() != &Mesh) {
            Existing->second->SetMeshData(Mesh.GetMeshData());
        }

        return true;
    }

    bool Engine::FAssetManager::SaveTexture(const std::filesystem::path& Path, const Engine::UTexture2D& Texture, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};

        if (Target.empty() || !Texture.IsValid() || Texture.GetPixels().empty()) {
            if (Error.empty()) {
                Error = "Only textures with stored pixel data can be saved";
            }

            return false;
        }

        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Engine::FTextureAssetData Data{Texture};
        Data.Serialize(Archive);

        if (Archive.HasError()) {
            Error = Archive.GetError();
            return false;
        }

        if (!Engine::FAssetFile::Save(Target, Engine::EAssetType::Texture, Payload, Error)) {
            return false;
        }

        const auto Existing{mTextures.find(CacheKey(GetRelativePath(Path, Error)))};

        if (Existing != mTextures.end() && Existing->second.get() != &Texture) {
            return Data.ApplyTo(*Existing->second, Error);
        }

        return true;
    }

    bool Engine::FAssetManager::ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, std::filesystem::path& ImportedPath, Core::FString& Error) {
        Error.clear();
        ImportedPath.clear();

        std::wstring Extension{Source.extension().wstring()};
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](wchar_t Character) {
            return static_cast<wchar_t>(std::towlower(Character));
        });
        const bool Texture{Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr"};

        if (!Texture && Extension != L".obj" && Extension != L".fbx" && Extension != L".gltf" && Extension != L".glb") {
            Error = "Unsupported import file type: " + Core::FilePath::ToUtf8(Source.filename());
            return false;
        }

        const auto Parent{Directory.empty() ? mContentRoot : ResolvePath(Directory, Error)};

        if (Parent.empty()) {
            return false;
        }

        std::error_code Code{};

        if (!std::filesystem::is_regular_file(Source, Code) || Code) {
            Error = "Cannot read the import source";
            return false;
        }

        std::filesystem::create_directories(Parent, Code);

        if (Code) {
            Error = "Cannot create the import destination: " + Code.message();
            return false;
        }

        const Core::FString Name{AssetFileName(Core::FilePath::ToUtf8(Source.stem()), "Asset")};
        const auto Base{Core::FilePath::FromUtf8(Name)};
        auto Folder{Parent / Base};

        for (Core::Uint32 Index{}; !std::filesystem::create_directory(Folder, Code); ++Index) {
            if (Code && Code != std::errc::file_exists) {
                Error = "Cannot create the import folder: " + Code.message();
                return false;
            }

            Code.clear();
            Folder = Parent / (Base.wstring() + L"_" + std::to_wstring(Index + 1));
        }

        Engine::FAssetImportScope Scope{Folder};
        auto Target{Folder / Base};
        Target += L".masset";

        if (!(Texture ? ImportTexture(Source, Target, Error) : ImportMesh(Source, Target, Error))) {
            const Core::FString Prefix{CacheKey(Core::FilePath::ToGenericUtf8(Folder.lexically_relative(mContentRoot))) + "/"};
            const auto RemoveCache{[&Prefix](auto& Cache) {
                std::erase_if(Cache, [&Prefix](const auto& Entry) {
                    return Entry.first.starts_with(Prefix);
                });
            }};
            RemoveCache(mMaterials);
            RemoveCache(mMeshes);
            RemoveCache(mNormalMaps);
            RemoveCache(mTextures);
            return false;
        }

        ImportedPath = Target.lexically_relative(mContentRoot);
        Scope.Commit();

        return true;
    }

    bool Engine::FAssetManager::ImportMesh(const std::filesystem::path& Source, const std::filesystem::path& Target, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::MeshImport};
        const auto Destination{ResolvePath(Target, Error)};

        if (Destination.empty()) {
            return false;
        }

        Engine::FMeshImportResult Imported{};

        if (!Engine::IMeshImporter::GetDefault().Import(Source, Imported, Error)) {
            Error = "Cannot import mesh source '" + Core::FilePath::ToUtf8(Source) + "': " + Error;
            return false;
        }

        Engine::FMeshData& Data{Imported.GetMeshData()};
        const auto& Materials{Imported.GetMaterials()};

        if (Materials.size() != Data.GetMaterialSlots().size()) {
            Error = "Imported materials do not match the mesh material slots";
            return false;
        }

        const auto MaterialDirectory{Destination.parent_path() / "Materials"};
        const auto TextureDirectory{Destination.parent_path() / "Textures"};
        Core::TMap<Core::FString, Core::FString> Textures{};

        for (std::size_t Index{}; Index < Materials.size(); ++Index) {
            const Engine::FMeshImportMaterial& SourceMaterial{Materials[Index]};
            auto Material{CoreUObject::NewObject<Engine::UMaterial>()};
            Material->SetName(SourceMaterial.GetName());

            if (!Material->SetBaseColor(SourceMaterial.GetBaseColor())) {
                Error = "Invalid imported material color: " + SourceMaterial.GetName();
                return false;
            }

            Material->SetTwoSided(SourceMaterial.IsTwoSided());

            Core::FString BaseColorPath{};
            Core::FString NormalMapPath{};

            if (!ImportMeshTexture(*this, SourceMaterial.GetBaseColorTexture(), TextureDirectory, Textures, BaseColorPath, Error) || !ImportMeshTexture(*this, SourceMaterial.GetNormalMapTexture(), TextureDirectory, Textures, NormalMapPath, Error)) {
                return false;
            }

            if (!Material->SetBaseColorTexturePath(BaseColorPath) || !Material->SetNormalMapTexturePath(NormalMapPath)) {
                Error = "Invalid imported material texture path: " + SourceMaterial.GetName();
                return false;
            }

            const auto MaterialTarget{UniqueAssetPath(MaterialDirectory, AssetFileName(Material->GetName(), "Material_" + std::to_string(Index)), Error)};

            if (MaterialTarget.empty() || !SaveMaterial(MaterialTarget, *Material, Error)) {
                return false;
            }

            const Core::FString MaterialPath{GetRelativePath(MaterialTarget, Error)};

            if (MaterialPath.empty()) {
                return false;
            }

            Data.GetMaterialSlots()[Index] = Engine::FMeshMaterialSlot{Material->GetName(), MaterialPath};
        }

        auto Mesh{CoreUObject::NewObject<Engine::UStaticMesh>()};

        if (!Mesh->SetMeshData(std::move(Data))) {
            Error = "Invalid imported mesh sections";
            return false;
        }

        if (!SaveMesh(Destination, *Mesh, Error)) {
            return false;
        }

        return true;
    }

    bool Engine::FAssetManager::ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::TextureImport};
        auto Texture{CoreUObject::NewObject<Engine::UTexture2D>()};

        if (!Texture->LoadFromFile(Source)) {
            Error = "Cannot import texture source";
            return false;
        }

        return SaveTexture(Target, *Texture, Error);
    }

    Engine::UMaterial* Engine::FAssetManager::LoadMaterial(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FScopedStatTimer StatTimer{Engine::EngineStats::MaterialLoad};
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mMaterials.find(Key)};

        if (Existing != mMaterials.end()) {
            Engine::EngineStats::AssetCacheHits.Add();
            return Existing->second.get();
        }

        auto Material{CoreUObject::NewObject<Engine::UMaterial>()};

        if (!Engine::FMaterialFile::Load(ResolvePath(Path, Error), *this, *Material, Error)) {
            return nullptr;
        }

        Engine::UMaterial* Result{Material.get()};

        mMaterials.emplace(Key, std::move(Material));

        return Result;
    }

    bool Engine::FAssetManager::SaveMaterial(const std::filesystem::path& Path, const Engine::UMaterial& Material, Core::FString& Error) {
        const auto Target{ResolvePath(Path, Error)};
        auto Candidate{CoreUObject::NewObject<Engine::UMaterial>()};

        if (Target.empty() || !ApplyMaterialSettings(*Candidate, Material, Error) || !Engine::FMaterialFile::Save(Target, *this, *Candidate, Error)) {
            return false;
        }

        const auto Existing{mMaterials.find(CacheKey(GetRelativePath(Path, Error)))};

        if (Existing != mMaterials.end() && Existing->second.get() != &Material) {
            return ApplyMaterialSettings(*Existing->second, Material, Error);
        }

        return true;
    }

    bool Engine::FAssetManager::ReloadMaterial(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return false;
        }

        const auto Existing{mMaterials.find(Key)};

        if (Existing == mMaterials.end()) {
            return LoadMaterial(Path, Error) != nullptr;
        }

        auto Candidate{CoreUObject::NewObject<Engine::UMaterial>()};

        if (!Engine::FMaterialFile::Load(ResolvePath(Path, Error), *this, *Candidate, Error)) {
            return false;
        }

        return ApplyMaterialSettings(*Existing->second, *Candidate, Error);
    }

    bool Engine::FAssetManager::ApplyMaterialSettings(Engine::UMaterial& Target, const Engine::UMaterial& Source, Core::FString& Error) {
        Target.SetName(Source.GetName());
        Target.SetShaderPath(Source.GetShaderPath());
        Target.SetBaseColor(Source.GetBaseColor());
        Target.SetBaseColorTexturePath(Source.GetBaseColorTexturePath());
        Target.SetNormalMapTexturePath(Source.GetNormalMapTexturePath());
        Target.SetSpecularStrength(Source.GetSpecularStrength());
        Target.SetShininess(Source.GetShininess());
        Target.SetNormalStrength(Source.GetNormalStrength());
        Target.SetTwoSided(Source.IsTwoSided());

        return Target.ResolveTexture(*this, Error);
    }

    Engine::UTexture2D* Engine::FAssetManager::LoadNormalMap(const std::filesystem::path& Path, Core::FString& Error) {
        const Core::FString Key{CacheKey(GetRelativePath(Path, Error))};

        if (Key.empty()) {
            return nullptr;
        }

        const auto Existing{mNormalMaps.find(Key)};

        if (Existing != mNormalMaps.end()) {
            return Existing->second.get();
        }

        Engine::UTexture2D* Source{LoadTexture(Path, Error)};

        if (Source == nullptr) {
            return nullptr;
        }

        Engine::FTextureAssetData Data{*Source};
        auto Texture{CoreUObject::NewObject<Engine::UTexture2D>()};

        if (!Data.ApplyTo(*Texture, Error) || !Texture->SetSRGB(false)) {
            Error = "Cannot create a linear normal map: " + Error;
            return nullptr;
        }

        Engine::UTexture2D* Result{Texture.get()};

        mNormalMaps.emplace(Key, std::move(Texture));

        return Result;
    }

    bool Engine::FAssetManager::MigrateMaterials(Core::FString& Error) {
        Error.clear();

        Core::TArray<std::filesystem::path> LegacyPaths{};
        Core::TArray<std::filesystem::path> AssetPaths{};
        std::error_code Code{};
        std::filesystem::recursive_directory_iterator Iterator{mContentRoot, Code};
        const std::filesystem::recursive_directory_iterator End{};

        while (!Code && Iterator != End) {
            if (Iterator->is_symlink(Code)) {
                Iterator.disable_recursion_pending();
            } else if (!Code && Iterator->is_regular_file(Code)) {
                const auto& Path{Iterator->path()};

                if (Path.extension() == L".mtl") {
                    LegacyPaths.push_back(Path);
                } else if (Path.extension() == L".masset") {
                    AssetPaths.push_back(Path);
                }
            }

            if (!Code) {
                Iterator.increment(Code);
            }
        }

        if (Code) {
            Error = "Cannot enumerate materials for migration: " + Code.message();
            return false;
        }

        if (LegacyPaths.empty()) {
            return true;
        }

        std::sort(LegacyPaths.begin(), LegacyPaths.end());
        std::sort(AssetPaths.begin(), AssetPaths.end());

        const auto Directory{mContentRoot.parent_path() / "Intermediate/MaterialMigration" / Core::FilePath::FromUtf8(Core::FGuid{}.ToString())};
        Engine::FAssetImportScope Scope{Directory};
        Core::TMap<Core::FString, Core::FString> Materials{};
        Core::TArray<std::filesystem::path> Targets{};

        for (const auto& Legacy : LegacyPaths) {
            const auto Source{ResolvePath(Legacy, Error)};

            if (Source.empty()) {
                return false;
            }

            auto Target{Source};
            Target.replace_extension(L".masset");
            const Core::FString Relative{GetRelativePath(Target, Error)};

            if (Relative.empty()) {
                return false;
            }

            Materials.emplace(CacheKey(GetRelativePath(Source, Error)), Relative);

            if (!IsLegacyMaterialAsset(Source)) {
                continue;
            }

            const bool Exists{std::filesystem::exists(Target, Code)};

            if (Code) {
                Error = "Cannot inspect material migration target: " + Code.message();
                return false;
            }

            if (Exists) {
                Engine::EAssetType Type{};

                if (!Engine::FAssetFile::GetType(Target, Type, Error) || Type != Engine::EAssetType::Material) {
                    Error = "Material migration target already contains another asset: " + Relative;
                    return false;
                }

                auto Material{CoreUObject::NewObject<Engine::UMaterial>()};

                if (!Engine::FMaterialFile::Load(Target, *this, *Material, Error)) {
                    return false;
                }

                continue;
            }

            auto Material{CoreUObject::NewObject<Engine::UMaterial>()};
            const auto Staged{Directory / (std::to_string(Targets.size()) + ".masset")};

            if (!Engine::FMaterialFile::LoadLegacy(Source, *this, *Material, Error) || !Engine::FMaterialFile::Save(Staged, *this, *Material, Error)) {
                Error = "Cannot migrate material '" + Core::FilePath::ToUtf8(Legacy) + "': " + Error;
                return false;
            }

            Targets.push_back(Target);
        }

        for (const auto& Path : AssetPaths) {
            Engine::EAssetType Type{};

            if (!Engine::FAssetFile::GetType(Path, Type, Error)) {
                return false;
            }

            if (Type != Engine::EAssetType::Mesh && Type != Engine::EAssetType::Scene) {
                continue;
            }

            Core::TArray<Core::Uint8> Payload{};

            if (ResolvePath(Path, Error).empty() || !Engine::FAssetFile::Load(Path, Type, Payload, Error)) {
                return false;
            }

            bool Changed{};

            if (Type == Engine::EAssetType::Mesh) {
                Serialization::FMemoryArchive Reader{static_cast<const Core::TArray<Core::Uint8>&>(Payload)};
                Engine::FMeshData Data{};
                SerializeMesh(Reader, Data);

                if (Reader.HasError() || Reader.GetRemainingBytes() != 0) {
                    Error = Reader.HasError() ? Reader.GetError() : "Invalid mesh data during material migration";
                    return false;
                }

                for (auto& Slot : Data.GetMaterialSlots()) {
                    Core::FString Reference{Slot.GetMaterialPath()};

                    if (!MigrateMaterialReference(Reference, *this, Materials, Directory, Targets, Changed, Error)) {
                        return false;
                    }

                    Slot = Engine::FMeshMaterialSlot{Slot.GetName(), std::move(Reference)};
                }

                if (Changed) {
                    auto Mesh{CoreUObject::NewObject<Engine::UStaticMesh>()};

                    if (!Mesh->SetMeshData(Data)) {
                        Error = "Invalid mesh settings during material migration";
                        return false;
                    }

                    Payload.clear();
                    Serialization::FMemoryArchive Writer{Payload};
                    SerializeMesh(Writer, Data);

                    if (Writer.HasError()) {
                        Error = Writer.GetError();
                        return false;
                    }
                }
            } else {
                rapidjson::Document Document{};
                Document.Parse<rapidjson::kParseValidateEncodingFlag>(reinterpret_cast<const char*>(Payload.data()), Payload.size());

                if (Document.HasParseError() || !Document.IsObject()) {
                    Error = "Invalid scene data during material migration";
                    return false;
                }

                if (!MigrateSceneReferences(Document, Document.GetAllocator(), *this, Materials, Directory, Targets, Changed, Error, false, 0)) {
                    return false;
                }

                if (Changed) {
                    rapidjson::StringBuffer Buffer{};
                    rapidjson::PrettyWriter<rapidjson::StringBuffer> Writer{Buffer};
                    Document.Accept(Writer);
                    const auto* Begin{reinterpret_cast<const Core::Uint8*>(Buffer.GetString())};
                    Payload.assign(Begin, Begin + Buffer.GetSize());
                }
            }

            if (Changed) {
                const auto Staged{Directory / (std::to_string(Targets.size()) + ".masset")};
                const auto Backup{Directory / "Backup" / Staged.filename()};
                std::filesystem::create_directories(Backup.parent_path(), Code);

                if (!Code) {
                    std::filesystem::copy_file(Path, Backup, Code);
                }

                if (Code || !Engine::FAssetFile::Save(Staged, Type, Payload, Error)) {
                    Error = Code ? "Cannot back up material references: " + Code.message() : Error;
                    return false;
                }

                Targets.push_back(Path);
            }
        }

        for (std::size_t Index{}; Index < Targets.size(); ++Index) {
            const auto Staged{Directory / (std::to_string(Index) + ".masset")};
            Engine::EAssetType Type{};
            Core::TArray<Core::Uint8> Payload{};

            if (Engine::FAssetFile::GetType(Staged, Type, Error) && Engine::FAssetFile::Load(Staged, Type, Payload, Error) && Engine::FAssetFile::Save(Targets[Index], Type, Payload, Error)) {
                continue;
            }

            const Core::FString Failure{Error};

            for (std::size_t Previous{}; Previous < Index; ++Previous) {
                const auto Backup{Directory / "Backup" / (std::to_string(Previous) + ".masset")};

                if (!std::filesystem::exists(Backup, Code)) {
                    continue;
                }

                if (!Engine::FAssetFile::GetType(Backup, Type, Error) || !Engine::FAssetFile::Load(Backup, Type, Payload, Error) || !Engine::FAssetFile::Save(Targets[Previous], Type, Payload, Error)) {
                    Scope.Commit();
                    Error = Failure + "; cannot restore material references. Backups: " + Core::FilePath::ToUtf8(Directory);
                    return false;
                }
            }

            Error = Failure;
            return false;
        }

        return true;
    }

}
