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
#include "CoreUObject/Reflection/NewObject.h"
#include "Serialization/MemoryArchive.h"
#include "FAssetSerialization.h"
#include "FTextureAssetData.h"
#include "FAssetImportScope.h"
#include "Engine/Import/IMeshImporter.h"
#include "Engine/Import/FMeshImportResult.h"

#include <algorithm>
#include <cwctype>
#include <limits>
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
            const auto Target{Directory / ("Texture_" + std::to_string(Textures.size()) + ".masset")};
            if (!Assets.ImportTexture(Source, Target, Error)) {
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
        if (Iterator->is_regular_file(Code) && (Iterator->path().extension() == L".masset" || Iterator->path().extension() == L".mtl")) {
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
    const auto DependencyDirectory{mContentRoot / ".Imported" / Destination.stem() / FilePath::FromUtf8(FGuid{}.ToString())};
    FAssetImportScope ImportScope{DependencyDirectory};
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
        if (!ImportMeshTexture(*this, SourceMaterial.GetBaseColorTexture(), DependencyDirectory, Textures, BaseColorPath, Error) || !ImportMeshTexture(*this, SourceMaterial.GetNormalMapTexture(), DependencyDirectory, Textures, NormalMapPath, Error)) {
            return false;
        }
        if (!Material->SetBaseColorTexturePath(BaseColorPath) || !Material->SetNormalMapTexturePath(NormalMapPath)) {
            Error = "Invalid imported material texture path: " + SourceMaterial.GetName();
            return false;
        }
        const auto MaterialTarget{DependencyDirectory / ("Material_" + std::to_string(Index) + ".mtl")};
        if (!SaveMaterial(MaterialTarget, *Material, Error)) {
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
    ImportScope.Commit();
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
    if (Target.empty() || !FMaterialFile::Save(Target, *this, Material, Error)) {
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
