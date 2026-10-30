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

/**
 * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
 */
namespace {
    /**
     * @brief 에셋 상대 경로를 대소문자 구분 없이 조회할 캐시 키로 변환합니다.
     * @param[in] Relative 캐시 키로 변환할 프로젝트 상대 에셋 경로입니다.
     * @return 입력 값을 변환한 문자열입니다.
     */
    FString CacheKey(const FString& Relative) {
        std::wstring Key{FilePath::FromUtf8(Relative).wstring()};

        for (wchar_t& Character : Key) {
            Character = static_cast<wchar_t>(std::towlower(Character));
        }

        return FilePath::ToGenericUtf8(std::filesystem::path{Key});
    }

    bool IsLegacyMaterialAsset(const std::filesystem::path& Path) {
        std::ifstream Input{Path};
        FString Line{};

        while (std::getline(Input, Line)) {
            std::istringstream Values{Line};
            FString Keyword{};
            Values >> Keyword;

            if (Keyword == "shader") {
                return true;
            }
        }

        return false;
    }

    FString AssetFileName(FString Name, const FString& Fallback) {
        for (char& Character : Name) {
            if (static_cast<unsigned char>(Character) < 32 || FString{"<>:\"/\\|?*"}.find(Character) != FString::npos) {
                Character = '_';
            }
        }

        while (!Name.empty() && (Name.back() == '.' || Name.back() == ' ')) {
            Name.pop_back();
        }

        if (Name.empty()) {
            Name = Fallback;
        }

        FString Base{Name.substr(0, Name.find('.'))};

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

    std::filesystem::path UniqueAssetPath(const std::filesystem::path& Directory, const FString& Name, FString& Error) {
        const auto Base{FilePath::FromUtf8(Name)};
        auto Target{Directory / Base};
        Target += L".masset";
        std::error_code Code{};

        for (Uint32 Index{1}; std::filesystem::exists(Target, Code) && !Code; ++Index) {
            Target = Directory / (Base.wstring() + L"_" + std::to_wstring(Index) + L".masset");
        }

        if (Code) {
            Error = "Cannot choose an asset filename: " + Code.message();
            return {};
        }

        return Target;
    }

    bool ImportMeshTexture(FAssetManager& Assets, const FMeshImportTexture& Texture, const std::filesystem::path& Directory, TMap<FString, FString>& Textures, FString& ImportedPath, FString& Error) {
        ImportedPath.clear();

        if (Texture.GetSource() == EMeshImportTextureSource::None) {
            return true;
        }

        if (Texture.GetSource() == EMeshImportTextureSource::Embedded) {
            Error = "Embedded material textures are not supported: " + Texture.GetEmbeddedReference();
            return false;
        }

        const auto& Source{Texture.GetFilePath()};
        const FString Key{CacheKey(FilePath::ToUtf8(Source))};
        auto Existing{Textures.find(Key)};

        if (Existing == Textures.end()) {
            const auto Target{UniqueAssetPath(Directory, AssetFileName(FilePath::ToUtf8(Source.stem()), "Texture"), Error)};

            if (Target.empty() || !Assets.ImportTexture(Source, Target, Error)) {
                Error = "Cannot import material texture '" + FilePath::ToUtf8(Source) + "': " + Error;
                return false;
            }

            const FString RelativePath{Assets.GetRelativePath(Target, Error)};

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
     * @param[in,out] Archive 로드 또는 저장 모드와 오류 상태를 제공하는 아카이브입니다.
     * @param[in,out] Mesh 읽거나 직렬화할 메시 데이터입니다.
     */
    void SerializeMesh(FMemoryArchive& Archive, FMeshData& Mesh) {
        FAssetSerialization::SerializeValues(Archive, Mesh.GetPositions());
        FAssetSerialization::SerializeValues(Archive, Mesh.GetNormals());
        FAssetSerialization::SerializeValues(Archive, Mesh.GetTangents());
        FAssetSerialization::SerializeValues(Archive, Mesh.GetColors());

        Uint32 Channels{Archive.IsLoading() ? 0 : static_cast<Uint32>(Mesh.GetUVChannels().size())};
        Archive.Serialize("UVChannels", Channels);

        if (Archive.HasError() || Channels > 8) {
            Archive.SetError("Invalid mesh UV channel count");
            return;
        }

        if (Archive.IsLoading()) {
            Mesh.GetUVChannels().resize(Channels);
        }

        for (auto& Channel : Mesh.GetUVChannels()) {
            FAssetSerialization::SerializeValues(Archive, Channel);
        }

        FAssetSerialization::SerializeValues(Archive, Mesh.GetIndices());

        if (Archive.HasError() || (Archive.IsLoading() && Archive.GetRemainingBytes() == 0)) {
            return;
        }

        Uint32 ExtensionVersion{1};
        Archive.Serialize("SectionsVersion", ExtensionVersion);

        if (ExtensionVersion != 1) {
            Archive.SetError("Unsupported mesh section version");
            return;
        }

        Uint32 SlotCount{Archive.IsLoading() ? 0 : static_cast<Uint32>(Mesh.GetMaterialSlots().size())};

        if (!Archive.BeginArray("MaterialSlots", SlotCount) || (Archive.IsLoading() && SlotCount > Archive.GetRemainingBytes() / 8)) {
            Archive.SetError("Invalid mesh material slot count");
            return;
        }

        if (Archive.IsLoading()) {
            Mesh.GetMaterialSlots().clear();
        }

        for (Uint32 Index{}; Index < SlotCount && !Archive.HasError(); ++Index) {
            FString Name{Archive.IsLoading() ? FString{} : Mesh.GetMaterialSlots()[Index].GetName()};
            FString Path{Archive.IsLoading() ? FString{} : Mesh.GetMaterialSlots()[Index].GetMaterialPath()};
            Archive.Serialize("Name", Name);
            Archive.Serialize("Material", Path);

            if (Archive.IsLoading()) {
                Mesh.GetMaterialSlots().emplace_back(std::move(Name), std::move(Path));
            }
        }

        Archive.EndArray();

        Uint32 SectionCount{Archive.IsLoading() ? 0 : static_cast<Uint32>(Mesh.GetSubMeshes().size())};

        if (!Archive.BeginArray("SubMeshes", SectionCount) || (Archive.IsLoading() && SectionCount > Archive.GetRemainingBytes() / 12)) {
            Archive.SetError("Invalid submesh count");
            return;
        }

        if (Archive.IsLoading()) {
            Mesh.GetSubMeshes().clear();
        }

        for (Uint32 Index{}; Index < SectionCount && !Archive.HasError(); ++Index) {
            Uint32 First{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetFirstIndex()};
            Uint32 Count{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetIndexCount()};
            Uint32 Slot{Archive.IsLoading() ? 0 : Mesh.GetSubMeshes()[Index].GetMaterialIndex()};
            Archive.Serialize("FirstIndex", First);
            Archive.Serialize("IndexCount", Count);
            Archive.Serialize("MaterialIndex", Slot);

            if (Archive.IsLoading()) {
                Mesh.GetSubMeshes().emplace_back(First, Count, Slot);
            }
        }

        Archive.EndArray();
    }

    bool MigrateMaterialReference(FString& Reference, FAssetManager& Assets, const TMap<FString, FString>& Materials, const std::filesystem::path& Directory, TArray<std::filesystem::path>& Targets, bool& Changed, FString& Error) {
        const auto Path{FilePath::FromUtf8(Reference)};

        if (Path.extension() != L".mtl") {
            return true;
        }

        const FString Key{CacheKey(Assets.GetRelativePath(Path, Error))};
        const auto Position{Materials.find(Key)};

        if (Key.empty() || Position == Materials.end()) {
            Error = "Cannot migrate material reference: " + Reference;
            return false;
        }

        const auto Target{Assets.ResolvePath(FilePath::FromUtf8(Position->second), Error)};
        std::error_code Code{};
        const bool Exists{!Target.empty() && std::filesystem::exists(Target, Code)};

        if (Target.empty() || Code) {
            Error = "Cannot inspect material reference migration target: " + Reference;
            return false;
        }

        if (Exists) {
            auto Legacy{NewObject<UMaterial>()};
            TArray<Uint8> Actual{};

            if (!FMaterialFile::LoadLegacy(Assets.ResolvePath(Path, Error), Assets, *Legacy, Error) || !FAssetFile::Load(Target, EAssetType::Material, Actual, Error)) {
                return false;
            }

            TArray<Uint8> Expected{};
            FMemoryArchive Writer{Expected};
            FMaterialAssetData Data{*Legacy};
            Data.Serialize(Writer);

            if (Writer.HasError() || Expected != Actual) {
                Error = "Material migration target conflicts with the legacy material: " + Position->second;
                return false;
            }
        } else if (std::find(Targets.begin(), Targets.end(), Target) == Targets.end()) {
            auto Legacy{NewObject<UMaterial>()};
            const auto Staged{Directory / (std::to_string(Targets.size()) + ".masset")};

            if (!FMaterialFile::LoadLegacy(Assets.ResolvePath(Path, Error), Assets, *Legacy, Error) || !FMaterialFile::Save(Staged, Assets, *Legacy, Error)) {
                return false;
            }

            Targets.push_back(Target);
        }

        Reference = Position->second;
        Changed = true;

        return true;
    }

    bool MigrateSceneReferences(rapidjson::Value& Value, rapidjson::Document::AllocatorType& Allocator, FAssetManager& Assets, const TMap<FString, FString>& Materials, const std::filesystem::path& Directory, TArray<std::filesystem::path>& Targets, bool& Changed, FString& Error, bool MaterialReference, Uint32 Depth) {
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
            FString Reference{Value.GetString(), Value.GetStringLength()};
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

FAssetManager::FAssetManager(std::filesystem::path ContentRoot)
	: mContentRoot{std::filesystem::absolute(ContentRoot).lexically_normal()} {
}

FAssetManager::~FAssetManager() = default;

const std::filesystem::path& FAssetManager::GetContentRoot() const {
    return mContentRoot;
}

FString FAssetManager::GetMeshPath(const UStaticMesh* Mesh) const {
    for (const auto& Entry : mMeshes) {
        if (Entry.second.get() == Mesh) {
            return Entry.first;
        }
    }

    return {};
}

FString FAssetManager::GetTexturePath(const UTexture2D* Texture) const {
    for (const auto& Entry : mTextures) {
        if (Entry.second.get() == Texture) {
            return Entry.first;
        }
    }

    return {};
}

std::filesystem::path FAssetManager::ResolvePath(const std::filesystem::path& Path, FString& Error) const {
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

FString FAssetManager::GetRelativePath(const std::filesystem::path& Path, FString& Error) const {
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

    return FilePath::ToGenericUtf8(Relative);
}

TArray<std::filesystem::path> FAssetManager::GetAssetPaths(FString& Error) const {
    Error.clear();

    TArray<std::filesystem::path> Paths{};
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

UStaticMesh* FAssetManager::LoadMesh(const std::filesystem::path& Path, FString& Error) {
    const FScopedStatTimer StatTimer{EngineStats::MeshLoad};
    const FString Key{CacheKey(GetRelativePath(Path, Error))};

    if (Key.empty()) {
        return nullptr;
    }

    const auto Existing{mMeshes.find(Key)};

    if (Existing != mMeshes.end()) {
        EngineStats::AssetCacheHits.Add();
        return Existing->second.get();
    }

    TArray<Uint8> Payload{};

    if (!FAssetFile::Load(ResolvePath(Path, Error), EAssetType::Mesh, Payload, Error)) {
        return nullptr;
    }

    FMemoryArchive Archive{static_cast<const TArray<Uint8>&>(Payload)};
    FMeshData Data{};
    SerializeMesh(Archive, Data);

    auto Mesh{NewObject<UStaticMesh>()};

    if (Archive.HasError() || Archive.GetRemainingBytes() != 0 || !Mesh->SetMeshData(std::move(Data))) {
        Error = Archive.HasError() ? Archive.GetError() : "Invalid mesh asset data";
        return nullptr;
    }

    UStaticMesh* Result{Mesh.get()};

    Mesh->SetName(FilePath::ToUtf8(Path.stem()));
    mMeshes.emplace(Key, std::move(Mesh));

    return Result;
}

UTexture2D* FAssetManager::LoadTexture(const std::filesystem::path& Path, FString& Error) {
    const FScopedStatTimer StatTimer{EngineStats::TextureLoad};
    const FString Key{CacheKey(GetRelativePath(Path, Error))};

    if (Key.empty()) {
        return nullptr;
    }

    const auto Existing{mTextures.find(Key)};

    if (Existing != mTextures.end()) {
        EngineStats::AssetCacheHits.Add();
        return Existing->second.get();
    }

    const auto Source{ResolvePath(Path, Error)};

    if (Source.empty()) {
        return nullptr;
    }

    if (Source.extension() != L".masset") {
        auto Texture{NewObject<UTexture2D>()};

        if (!Texture->LoadFromFile(Source)) {
            Error = "Cannot load the material texture";
            return nullptr;
        }

        UTexture2D* Result{Texture.get()};

        mTextures.emplace(Key, std::move(Texture));

        return Result;
    }

    TArray<Uint8> Payload{};

    if (!FAssetFile::Load(ResolvePath(Path, Error), EAssetType::Texture, Payload, Error)) {
        return nullptr;
    }

    FMemoryArchive Archive{static_cast<const TArray<Uint8>&>(Payload)};
    FTextureAssetData Data{};
    Data.Serialize(Archive);

    auto Texture{NewObject<UTexture2D>()};

    if (Archive.HasError() || Archive.GetRemainingBytes() != 0) {
        Error = Archive.HasError() ? Archive.GetError() : "Invalid texture asset data";
        return nullptr;
    }

    if (!Data.ApplyTo(*Texture, Error)) {
        return nullptr;
    }

    UTexture2D* Result{Texture.get()};

    mTextures.emplace(Key, std::move(Texture));

    return Result;
}

bool FAssetManager::SaveMesh(const std::filesystem::path& Path, const UStaticMesh& Mesh, FString& Error) {
    const auto Target{ResolvePath(Path, Error)};

    if (Target.empty() || !Mesh.IsValid()) {
        if (Error.empty()) {
            Error = "Cannot save an invalid mesh";
        }

        return false;
    }

    TArray<Uint8> Payload{};
    FMemoryArchive Archive{Payload};
    FMeshData Data{Mesh.GetMeshData()};
    SerializeMesh(Archive, Data);

    if (Archive.HasError()) {
        Error = Archive.GetError();
        return false;
    }

    if (!FAssetFile::Save(Target, EAssetType::Mesh, Payload, Error)) {
        return false;
    }

    const auto Existing{mMeshes.find(CacheKey(GetRelativePath(Path, Error)))};

    if (Existing != mMeshes.end() && Existing->second.get() != &Mesh) {
        Existing->second->SetMeshData(Mesh.GetMeshData());
    }

    return true;
}

bool FAssetManager::SaveTexture(const std::filesystem::path& Path, const UTexture2D& Texture, FString& Error) {
    const auto Target{ResolvePath(Path, Error)};

    if (Target.empty() || !Texture.IsValid() || Texture.GetPixels().empty()) {
        if (Error.empty()) {
            Error = "Only textures with stored pixel data can be saved";
        }

        return false;
    }

    TArray<Uint8> Payload{};
    FMemoryArchive Archive{Payload};
    FTextureAssetData Data{Texture};
    Data.Serialize(Archive);

    if (Archive.HasError()) {
        Error = Archive.GetError();
        return false;
    }

    if (!FAssetFile::Save(Target, EAssetType::Texture, Payload, Error)) {
        return false;
    }

    const auto Existing{mTextures.find(CacheKey(GetRelativePath(Path, Error)))};

    if (Existing != mTextures.end() && Existing->second.get() != &Texture) {
        return Data.ApplyTo(*Existing->second, Error);
    }

    return true;
}

bool FAssetManager::ImportAsset(const std::filesystem::path& Source, const std::filesystem::path& Directory, std::filesystem::path& ImportedPath, FString& Error) {
    Error.clear();
    ImportedPath.clear();

    std::wstring Extension{Source.extension().wstring()};
    std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](wchar_t Character) {
        return static_cast<wchar_t>(std::towlower(Character));
    });
    const bool Texture{Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr"};

    if (!Texture && Extension != L".obj" && Extension != L".fbx" && Extension != L".gltf" && Extension != L".glb") {
        Error = "Unsupported import file type: " + FilePath::ToUtf8(Source.filename());
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

    const FString Name{AssetFileName(FilePath::ToUtf8(Source.stem()), "Asset")};
    const auto Base{FilePath::FromUtf8(Name)};
    auto Folder{Parent / Base};

    for (Uint32 Index{}; !std::filesystem::create_directory(Folder, Code); ++Index) {
        if (Code && Code != std::errc::file_exists) {
            Error = "Cannot create the import folder: " + Code.message();
            return false;
        }

        Code.clear();
        Folder = Parent / (Base.wstring() + L"_" + std::to_wstring(Index + 1));
    }

    FAssetImportScope Scope{Folder};
    auto Target{Folder / Base};
    Target += L".masset";

    if (!(Texture ? ImportTexture(Source, Target, Error) : ImportMesh(Source, Target, Error))) {
        const FString Prefix{CacheKey(FilePath::ToGenericUtf8(Folder.lexically_relative(mContentRoot))) + "/"};
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

bool FAssetManager::ImportMesh(const std::filesystem::path& Source, const std::filesystem::path& Target, FString& Error) {
    const FScopedStatTimer StatTimer{EngineStats::MeshImport};
    const auto Destination{ResolvePath(Target, Error)};

    if (Destination.empty()) {
        return false;
    }

    FMeshImportResult Imported{};

    if (!IMeshImporter::GetDefault().Import(Source, Imported, Error)) {
        Error = "Cannot import mesh source '" + FilePath::ToUtf8(Source) + "': " + Error;
        return false;
    }

    FMeshData& Data{Imported.GetMeshData()};
    const auto& Materials{Imported.GetMaterials()};

    if (Materials.size() != Data.GetMaterialSlots().size()) {
        Error = "Imported materials do not match the mesh material slots";
        return false;
    }

    const auto MaterialDirectory{Destination.parent_path() / "Materials"};
    const auto TextureDirectory{Destination.parent_path() / "Textures"};
    TMap<FString, FString> Textures{};

    for (std::size_t Index{}; Index < Materials.size(); ++Index) {
        const FMeshImportMaterial& SourceMaterial{Materials[Index]};
        auto Material{NewObject<UMaterial>()};
        Material->SetName(SourceMaterial.GetName());

        if (!Material->SetBaseColor(SourceMaterial.GetBaseColor())) {
            Error = "Invalid imported material color: " + SourceMaterial.GetName();
            return false;
        }

        Material->SetTwoSided(SourceMaterial.IsTwoSided());

        FString BaseColorPath{};
        FString NormalMapPath{};

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

        const FString MaterialPath{GetRelativePath(MaterialTarget, Error)};

        if (MaterialPath.empty()) {
            return false;
        }

        Data.GetMaterialSlots()[Index] = FMeshMaterialSlot{Material->GetName(), MaterialPath};
    }

    auto Mesh{NewObject<UStaticMesh>()};

    if (!Mesh->SetMeshData(std::move(Data))) {
        Error = "Invalid imported mesh sections";
        return false;
    }

    if (!SaveMesh(Destination, *Mesh, Error)) {
        return false;
    }

    return true;
}

bool FAssetManager::ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, FString& Error) {
    const FScopedStatTimer StatTimer{EngineStats::TextureImport};
    auto Texture{NewObject<UTexture2D>()};

    if (!Texture->LoadFromFile(Source)) {
        Error = "Cannot import texture source";
        return false;
    }

    return SaveTexture(Target, *Texture, Error);
}

UMaterial* FAssetManager::LoadMaterial(const std::filesystem::path& Path, FString& Error) {
    const FScopedStatTimer StatTimer{EngineStats::MaterialLoad};
    const FString Key{CacheKey(GetRelativePath(Path, Error))};

    if (Key.empty()) {
        return nullptr;
    }

    const auto Existing{mMaterials.find(Key)};

    if (Existing != mMaterials.end()) {
        EngineStats::AssetCacheHits.Add();
        return Existing->second.get();
    }

    auto Material{NewObject<UMaterial>()};

    if (!FMaterialFile::Load(ResolvePath(Path, Error), *this, *Material, Error)) {
        return nullptr;
    }

    UMaterial* Result{Material.get()};

    mMaterials.emplace(Key, std::move(Material));

    return Result;
}

bool FAssetManager::SaveMaterial(const std::filesystem::path& Path, const UMaterial& Material, FString& Error) {
    const auto Target{ResolvePath(Path, Error)};
    auto Candidate{NewObject<UMaterial>()};

    if (Target.empty() || !ApplyMaterialSettings(*Candidate, Material, Error) || !FMaterialFile::Save(Target, *this, *Candidate, Error)) {
        return false;
    }

    const auto Existing{mMaterials.find(CacheKey(GetRelativePath(Path, Error)))};

    if (Existing != mMaterials.end() && Existing->second.get() != &Material) {
        return ApplyMaterialSettings(*Existing->second, Material, Error);
    }

    return true;
}

bool FAssetManager::ReloadMaterial(const std::filesystem::path& Path, FString& Error) {
    const FString Key{CacheKey(GetRelativePath(Path, Error))};

    if (Key.empty()) {
        return false;
    }

    const auto Existing{mMaterials.find(Key)};

    if (Existing == mMaterials.end()) {
        return LoadMaterial(Path, Error) != nullptr;
    }

    auto Candidate{NewObject<UMaterial>()};

    if (!FMaterialFile::Load(ResolvePath(Path, Error), *this, *Candidate, Error)) {
        return false;
    }

    return ApplyMaterialSettings(*Existing->second, *Candidate, Error);
}

bool FAssetManager::ApplyMaterialSettings(UMaterial& Target, const UMaterial& Source, FString& Error) {
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

UTexture2D* FAssetManager::LoadNormalMap(const std::filesystem::path& Path, FString& Error) {
    const FString Key{CacheKey(GetRelativePath(Path, Error))};

    if (Key.empty()) {
        return nullptr;
    }

    const auto Existing{mNormalMaps.find(Key)};

    if (Existing != mNormalMaps.end()) {
        return Existing->second.get();
    }

    UTexture2D* Source{LoadTexture(Path, Error)};

    if (Source == nullptr) {
        return nullptr;
    }

    FTextureAssetData Data{*Source};
    auto Texture{NewObject<UTexture2D>()};

    if (!Data.ApplyTo(*Texture, Error) || !Texture->SetSRGB(false)) {
        Error = "Cannot create a linear normal map: " + Error;
        return nullptr;
    }

    UTexture2D* Result{Texture.get()};

    mNormalMaps.emplace(Key, std::move(Texture));

    return Result;
}

bool FAssetManager::MigrateMaterials(FString& Error) {
    Error.clear();

    TArray<std::filesystem::path> LegacyPaths{};
    TArray<std::filesystem::path> AssetPaths{};
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

    const auto Directory{mContentRoot.parent_path() / "Intermediate/MaterialMigration" / FilePath::FromUtf8(FGuid{}.ToString())};
    FAssetImportScope Scope{Directory};
    TMap<FString, FString> Materials{};
    TArray<std::filesystem::path> Targets{};

    for (const auto& Legacy : LegacyPaths) {
        const auto Source{ResolvePath(Legacy, Error)};

        if (Source.empty()) {
            return false;
        }

        auto Target{Source};
        Target.replace_extension(L".masset");
        const FString Relative{GetRelativePath(Target, Error)};

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
            EAssetType Type{};

            if (!FAssetFile::GetType(Target, Type, Error) || Type != EAssetType::Material) {
                Error = "Material migration target already contains another asset: " + Relative;
                return false;
            }

            auto Material{NewObject<UMaterial>()};

            if (!FMaterialFile::Load(Target, *this, *Material, Error)) {
                return false;
            }

            continue;
        }

        auto Material{NewObject<UMaterial>()};
        const auto Staged{Directory / (std::to_string(Targets.size()) + ".masset")};

        if (!FMaterialFile::LoadLegacy(Source, *this, *Material, Error) || !FMaterialFile::Save(Staged, *this, *Material, Error)) {
            Error = "Cannot migrate material '" + FilePath::ToUtf8(Legacy) + "': " + Error;
            return false;
        }

        Targets.push_back(Target);
    }

    for (const auto& Path : AssetPaths) {
        EAssetType Type{};

        if (!FAssetFile::GetType(Path, Type, Error)) {
            return false;
        }

        if (Type != EAssetType::Mesh && Type != EAssetType::Scene) {
            continue;
        }

        TArray<Uint8> Payload{};

        if (ResolvePath(Path, Error).empty() || !FAssetFile::Load(Path, Type, Payload, Error)) {
            return false;
        }

        bool Changed{};

        if (Type == EAssetType::Mesh) {
            FMemoryArchive Reader{static_cast<const TArray<Uint8>&>(Payload)};
            FMeshData Data{};
            SerializeMesh(Reader, Data);

            if (Reader.HasError() || Reader.GetRemainingBytes() != 0) {
                Error = Reader.HasError() ? Reader.GetError() : "Invalid mesh data during material migration";
                return false;
            }

            for (auto& Slot : Data.GetMaterialSlots()) {
                FString Reference{Slot.GetMaterialPath()};

                if (!MigrateMaterialReference(Reference, *this, Materials, Directory, Targets, Changed, Error)) {
                    return false;
                }

                Slot = FMeshMaterialSlot{Slot.GetName(), std::move(Reference)};
            }

            if (Changed) {
                auto Mesh{NewObject<UStaticMesh>()};

                if (!Mesh->SetMeshData(Data)) {
                    Error = "Invalid mesh settings during material migration";
                    return false;
                }

                Payload.clear();
                FMemoryArchive Writer{Payload};
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
                const auto* Begin{reinterpret_cast<const Uint8*>(Buffer.GetString())};
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

            if (Code || !FAssetFile::Save(Staged, Type, Payload, Error)) {
                Error = Code ? "Cannot back up material references: " + Code.message() : Error;
                return false;
            }

            Targets.push_back(Path);
        }
    }

    for (std::size_t Index{}; Index < Targets.size(); ++Index) {
        const auto Staged{Directory / (std::to_string(Index) + ".masset")};
        EAssetType Type{};
        TArray<Uint8> Payload{};

        if (FAssetFile::GetType(Staged, Type, Error) && FAssetFile::Load(Staged, Type, Payload, Error) && FAssetFile::Save(Targets[Index], Type, Payload, Error)) {
            continue;
        }

        const FString Failure{Error};

        for (std::size_t Previous{}; Previous < Index; ++Previous) {
            const auto Backup{Directory / "Backup" / (std::to_string(Previous) + ".masset")};

            if (!std::filesystem::exists(Backup, Code)) {
                continue;
            }

            if (!FAssetFile::GetType(Backup, Type, Error) || !FAssetFile::Load(Backup, Type, Payload, Error) || !FAssetFile::Save(Targets[Previous], Type, Payload, Error)) {
                Scope.Commit();
                Error = Failure + "; cannot restore material references. Backups: " + FilePath::ToUtf8(Directory);
                return false;
            }
        }

        Error = Failure;
        return false;
    }

    return true;
}
