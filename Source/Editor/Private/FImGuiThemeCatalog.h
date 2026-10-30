#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <imgui.h>
#include <filesystem>

class FImGuiThemeCatalog {
public:
    ReflectTypes(FImGuiThemeCatalog);

public:
    FImGuiThemeCatalog();

public:
    ReflectStruct(FImGuiThemeCatalog);

    bool Load(const std::filesystem::path& Path, FString& Error);
    Uint32 GetCount() const;
    Uint32 Find(const FString& Name) const;
    const FString& GetName(Uint32 Index) const;
    const FString& GetAuthor(Uint32 Index) const;
    const FString& GetDescription(Uint32 Index) const;
    const ImGuiStyle& GetStyle(Uint32 Index) const;

private:
    ReflectMembers();

private:
    TArray<FString> mNames{};
    ReflectProperty(mNames, "Names", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FString> mAuthors{};
    ReflectProperty(mAuthors, "Authors", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FString> mDescriptions{};
    ReflectProperty(mDescriptions, "Descriptions", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<ImGuiStyle> mStyles{};
    ReflectProperty(mStyles, "Styles", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
