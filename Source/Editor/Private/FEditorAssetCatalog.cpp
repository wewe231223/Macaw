/**
 * @file FEditorAssetCatalog.cpp
 * @brief 실제 Content 경로를 사용하는 에셋 목록의 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FEditorAssetCatalog.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"

#include <algorithm>
#include <cwctype>

namespace Editor {

    Editor::FEditorAssetEntry::FEditorAssetEntry(std::filesystem::path Path, std::optional<Engine::EAssetType> Type)
		: mPath{std::move(Path)},
		  mType{Type} {
    }

    const std::filesystem::path& Editor::FEditorAssetEntry::GetPath() const {
        return mPath;
    }

    std::optional<Engine::EAssetType> Editor::FEditorAssetEntry::GetType() const {
        return mType;
    }

    Editor::FEditorAssetCatalog::FEditorAssetCatalog(Engine::FAssetManager* Assets)
		: mAssets{Assets} {
    }

    Core::FString Editor::FEditorAssetCatalog::MakeKey(const std::filesystem::path& Path) {
        std::wstring Key{Path.lexically_normal().generic_wstring()};

        for (wchar_t& Character : Key) {
            Character = static_cast<wchar_t>(std::towlower(Character));
        }

        return Core::FilePath::ToUtf8(std::filesystem::path{Key});
    }

    bool Editor::FEditorAssetCatalog::Refresh(Core::FString& Error) {
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
            Core::FString PathError{};

            if (Link || mAssets->ResolvePath(Iterator->path(), PathError).empty()) {
                Iterator.disable_recursion_pending();
            } else if (Directory) {
                mDirectories.push_back(Path);
            } else if (!Code && Iterator->is_regular_file(Code)) {
                std::wstring Extension{Path.extension().wstring()};
                std::transform(Extension.begin(), Extension.end(), Extension.begin(), [](wchar_t Value) {
                    return static_cast<wchar_t>(std::towlower(Value));
                });

                std::optional<Engine::EAssetType> Type{};

                if (Extension == L".masset") {
                    Engine::EAssetType Value{};

                    if (Engine::FAssetFile::GetType(Iterator->path(), Value, PathError)) {
                        Type = Value;
                    } else if (Error.empty()) {
                        Error = Core::FilePath::ToUtf8(Path) + ": " + PathError;
                    }
                } else if (Extension == L".png" || Extension == L".jpg" || Extension == L".jpeg" || Extension == L".bmp" || Extension == L".tga" || Extension == L".dds" || Extension == L".hdr") {
                    Type = Engine::EAssetType::Texture;
                }

                mEntries.emplace_back(Path, Type);
            }

            if (!Code) {
                Iterator.increment(Code);
            }
        }

        std::sort(mEntries.begin(), mEntries.end(), [](const Editor::FEditorAssetEntry& First, const Editor::FEditorAssetEntry& Second) {
            return First.GetPath() < Second.GetPath();
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

    const Core::TArray<Editor::FEditorAssetEntry>& Editor::FEditorAssetCatalog::GetEntries() const {
        return mEntries;
    }

    const Core::TArray<std::filesystem::path>& Editor::FEditorAssetCatalog::GetDirectories() const {
        return mDirectories;
    }

    const Editor::FEditorAssetEntry* Editor::FEditorAssetCatalog::Find(const std::filesystem::path& Path) const {
        Core::FString Error{};
        const Core::FString Relative{mAssets != nullptr ? mAssets->GetRelativePath(Path, Error) : Core::FString{}};
        const auto Position{mIndices.find(MakeKey(Core::FilePath::FromUtf8(Relative)))};
        return Position == mIndices.end() ? nullptr : &mEntries[Position->second];
    }

    Engine::FAssetManager* Editor::FEditorAssetCatalog::GetAssets() const {
        return mAssets;
    }

}
