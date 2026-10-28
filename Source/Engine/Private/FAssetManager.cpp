/**
 * @file FAssetManager.cpp
 * @brief 프로젝트 콘텐츠 경로와 메시·텍스처 에셋의 로드 및 캐시에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FAssetManager.h"
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
    }
}

FAssetManager::FAssetManager(std::filesystem::path ContentRoot)
	: mContentRoot{std::filesystem::absolute(ContentRoot).lexically_normal()} {
}

FAssetManager::~FAssetManager() = default;

const std::filesystem::path& FAssetManager::GetContentRoot() const {
    return mContentRoot;
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
    const FString Key{CacheKey(GetRelativePath(Path, Error))};
    if (Key.empty()) {
        return nullptr;
    }
    const auto Existing{mMeshes.find(Key)};
    if (Existing != mMeshes.end()) {
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
    mMeshes.emplace(Key, std::move(Mesh));
    return Result;
}

UTexture2D* FAssetManager::LoadTexture(const std::filesystem::path& Path, FString& Error) {
    const FString Key{CacheKey(GetRelativePath(Path, Error))};
    if (Key.empty()) {
        return nullptr;
    }
    const auto Existing{mTextures.find(Key)};
    if (Existing != mTextures.end()) {
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
    auto Mesh{NewObject<UStaticMesh>()};
    if (!Mesh->LoadFromFile(Source)) {
        Error = "Cannot import mesh source";
        return false;
    }
    return SaveMesh(Target, *Mesh, Error);
}

bool FAssetManager::ImportTexture(const std::filesystem::path& Source, const std::filesystem::path& Target, FString& Error) {
    auto Texture{NewObject<UTexture2D>()};
    if (!Texture->LoadFromFile(Source)) {
        Error = "Cannot import texture source";
        return false;
    }
    return SaveTexture(Target, *Texture, Error);
}

UMaterial* FAssetManager::LoadMaterial(const std::filesystem::path& Path, FString& Error) {
    const FString Key{CacheKey(GetRelativePath(Path, Error))};
    if (Key.empty()) {
        return nullptr;
    }
    const auto Existing{mMaterials.find(Key)};
    if (Existing != mMaterials.end()) {
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
    Target.SetTwoSided(Source.IsTwoSided());
    return Target.ResolveTexture(*this, Error);
}
