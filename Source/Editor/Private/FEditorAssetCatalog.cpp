/**
 * @file FEditorAssetCatalog.cpp
 * @brief 임포트 내부 경로를 감춘 에셋 목록의 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FEditorAssetCatalog.h"
#include "Core/FilePath.h"
#include "Core/Guid.h"
#include "Engine/FAssetManager.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UMaterial.h"

#include <algorithm>
#include <cwctype>
#include <set>

FEditorAssetEntry::FEditorAssetEntry(std::filesystem::path Path, std::filesystem::path DisplayPath, std::optional<EAssetType> Type)
	: mPath{std::move(Path)},
	  mDisplayPath{std::move(DisplayPath)},
	  mType{Type} {
}

const std::filesystem::path& FEditorAssetEntry::GetPath() const {
    return mPath;
}

const std::filesystem::path& FEditorAssetEntry::GetDisplayPath() const {
    return mDisplayPath;
}

std::optional<EAssetType> FEditorAssetEntry::GetType() const {
    return mType;
}

FEditorAssetCatalog::FEditorAssetCatalog(FAssetManager* Assets)
	: mAssets{Assets} {
}

FString FEditorAssetCatalog::MakeKey(const std::filesystem::path& Path) {
    std::wstring Key{Path.lexically_normal().generic_wstring()};

    for (wchar_t& Character : Key) {
        Character = static_cast<wchar_t>(std::towlower(Character));
    }

    return FilePath::ToUtf8(std::filesystem::path{Key});
}

bool FEditorAssetCatalog::IsImportStorage(const std::filesystem::path& Path) {
    for (auto Part{Path.begin()}; Part != Path.end(); ++Part) {
        if (*Part == L".Imported") {
            return true;
        }

        if (Part->wstring().ends_with(L"_Assets")) {
            auto Next{Part};
            ++Next;

            FGuid Guid{};

            if (Next != Path.end() && FGuid::Parse(FilePath::ToUtf8(*Next), Guid)) {
                return true;
            }
        }
    }

    return false;
}

void FEditorAssetCatalog::Add(const std::filesystem::path& Path, const std::filesystem::path& DisplayPath, std::optional<EAssetType> Type) {
    FString Error{};
    const FString Relative{mAssets->GetRelativePath(Path, Error)};

    if (Relative.empty()) {
        return;
    }

    const FString Key{MakeKey(FilePath::FromUtf8(Relative))};

    if (mIndices.contains(Key)) {
        return;
    }

    mIndices.emplace(Key, mEntries.size());
    mEntries.emplace_back(FilePath::FromUtf8(Relative), DisplayPath, Type);

    for (auto Directory{DisplayPath.parent_path()}; !Directory.empty(); Directory = Directory.parent_path()) {
        mDirectories.push_back(Directory);
    }
}

void FEditorAssetCatalog::AddMeshDependencies(const FEditorAssetEntry& Entry, FString& Error) {
    UStaticMesh* Mesh{mAssets->LoadMesh(Entry.GetPath(), Error)};

    if (Mesh == nullptr) {
        return;
    }

    std::set<Uint32> UsedSlots{};

    for (const FSubMesh& SubMesh : Mesh->GetMeshData().GetSubMeshes()) {
        UsedSlots.insert(SubMesh.GetMaterialIndex());
    }

    for (Uint32 Index : UsedSlots) {
        const auto& Slot{Mesh->GetMeshData().GetMaterialSlots()[Index]};

        if (Slot.GetMaterialPath().empty()) {
            continue;
        }

        const auto Path{FilePath::FromUtf8(Slot.GetMaterialPath())};
        UMaterial* Material{mAssets->LoadMaterial(Path, Error)};

        if (Material == nullptr) {
            continue;
        }

        if (IsImportStorage(Path)) {
            FString Name{Material->GetName()};

            for (char& Character : Name) {
                if (FString{"<>:\"/\\|?*"}.find(Character) != FString::npos) {
                    Character = '_';
                }
            }

            Add(Path, std::filesystem::path{"Materials"} / Entry.GetDisplayPath().stem() / FilePath::FromUtf8(Name + ".mtl"), EAssetType::Material);
        }

        const auto Texture{FilePath::FromUtf8(Material->GetBaseColorTexturePath())};

        if (!Texture.empty() && IsImportStorage(Texture)) {
            Add(Texture, std::filesystem::path{"Textures"} / Entry.GetDisplayPath().stem() / Texture.filename(), EAssetType::Texture);
        }
    }
}

bool FEditorAssetCatalog::Refresh(FString& Error) {
    Error.clear();
    mEntries.clear();
    mIndices.clear();
    mDirectories.clear();

    if (mAssets == nullptr) {
        Error = "The asset catalog requires an open project";
        return false;
    }

    const auto& Root{mAssets->GetContentRoot()};
    std::error_code Code{};
    std::filesystem::recursive_directory_iterator Iterator{Root, Code};
    const std::filesystem::recursive_directory_iterator End{};

    while (!Code && Iterator != End) {
        const auto Path{Iterator->path().lexically_relative(Root)};
        const bool Link{Iterator->is_symlink(Code)};
        const bool Directory{!Code && Iterator->is_directory(Code)};
        bool Storage{IsImportStorage(Path)};

        if (Directory && Path.filename().wstring().ends_with(L"_Assets")) {
            std::error_code ChildError{};

            for (std::filesystem::directory_iterator Child{Iterator->path(), ChildError}; !ChildError && Child != std::filesystem::directory_iterator{}; Child.increment(ChildError)) {
                FGuid Guid{};

                if (Child->is_directory(ChildError) && FGuid::Parse(FilePath::ToUtf8(Child->path().filename()), Guid)) {
                    Storage = true;
                    break;
                }
            }
        }

        FString PathError{};

        if (Link || Storage || mAssets->ResolvePath(Iterator->path(), PathError).empty()) {
            Iterator.disable_recursion_pending();
        } else if (Directory) {
            mDirectories.push_back(Path);
        } else if (!Code && Iterator->is_regular_file(Code)) {
            std::wstring Extension{Path.extension().wstring()};
            std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](wchar_t Value) {
                return static_cast<wchar_t>(std::towlower(Value));
            });

            std::optional<EAssetType> Type{};

            if (Extension == L".masset" || Extension == L".mtl") {
                EAssetType Value{};

                if (FAssetFile::GetType(Iterator->path(), Value, PathError)) {
                    Type = Value;
                } else if (Error.empty()) {
                    Error = FilePath::ToUtf8(Path) + ": " + PathError;
                }
            } else if (Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr") {
                Type = EAssetType::Texture;
            }

            Add(Path, Path, Type);
        }

        if (!Code) {
            Iterator.increment(Code);
        }
    }

    TArray<FEditorAssetEntry> Meshes{};

    for (const auto& Entry : mEntries) {
        if (Entry.GetType() == EAssetType::Mesh) {
            Meshes.push_back(Entry);
        }
    }

    for (const auto& Entry : Meshes) {
        FString DependencyError{};
        AddMeshDependencies(Entry, DependencyError);

        if (Error.empty() && !DependencyError.empty()) {
            Error = FilePath::ToUtf8(Entry.GetPath()) + ": " + DependencyError;
        }
    }

    std::sort(mEntries.begin(), mEntries.end(), [](const FEditorAssetEntry& First, const FEditorAssetEntry& Second) {
        return First.GetDisplayPath() < Second.GetDisplayPath();
    });

    mIndices.clear();

    for (std::size_t Index{}; Index < mEntries.size(); ++Index) {
        mIndices.emplace(MakeKey(mEntries[Index].GetPath()), Index);
    }

    std::sort(mDirectories.begin(), mDirectories.end());
    mDirectories.erase(std::unique(mDirectories.begin(), mDirectories.end()), mDirectories.end());

    if (Code) {
        Error = "Cannot read Content: " + Code.message();
    }

    return !Code;
}

const TArray<FEditorAssetEntry>& FEditorAssetCatalog::GetEntries() const {
    return mEntries;
}

const TArray<std::filesystem::path>& FEditorAssetCatalog::GetDirectories() const {
    return mDirectories;
}

const FEditorAssetEntry* FEditorAssetCatalog::Find(const std::filesystem::path& Path) const {
    FString Error{};
    const FString Relative{mAssets != nullptr ? mAssets->GetRelativePath(Path, Error) : FString{}};
    const auto Position{mIndices.find(MakeKey(FilePath::FromUtf8(Relative)))};
    return Position == mIndices.end() ? nullptr : &mEntries[Position->second];
}

FString FEditorAssetCatalog::GetDisplayPath(const std::filesystem::path& Path) const {
    if (const auto* Entry{Find(Path)}) {
        return FilePath::ToGenericUtf8(Entry->GetDisplayPath());
    }

    if (mAssets != nullptr && Path.extension() == L".mtl") {
        FString Error{};

        if (const UMaterial* Material{mAssets->LoadMaterial(Path, Error)}) {
            return Material->GetName();
        }
    }

    return FilePath::ToUtf8(Path.filename());
}

FAssetManager* FEditorAssetCatalog::GetAssets() const {
    return mAssets;
}
