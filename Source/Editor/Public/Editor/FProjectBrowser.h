#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <array>
#include <filesystem>
#include <vector>

class FWindowsApplication;
class FRenderer;

class FProjectBrowser {
public:
    ReflectTypes(FProjectBrowser);

public:
    FProjectBrowser();

public:
    ReflectStruct(FProjectBrowser);
    bool Show(FWindowsApplication& Platform, FRenderer& Renderer, std::filesystem::path& ProjectFile, bool& Create, const FString& Error);
    static void RememberProject(const std::filesystem::path& ProjectFile);

private:
    void Draw(FWindowsApplication& Platform, const FString& Error);
    void DrawOpenProject(FWindowsApplication& Platform);
    void DrawNewProject(FWindowsApplication& Platform);
    void LoadRecentProjects();
    std::filesystem::path GetNewProjectFile() const;
    bool IsProjectNameValid() const;
    ReflectMembers();

private:
    std::array<char, 256> mProjectName{};
    ReflectProperty(mProjectName, "ProjectName", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::array<char, 4096> mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::vector<std::filesystem::path> mRecentProjects{};
    ReflectProperty(mRecentProjects, "RecentProjects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::filesystem::path mSelectedFile{};
    ReflectProperty(mSelectedFile, "SelectedFile", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mCreatePage{};
    ReflectProperty(mCreatePage, "CreatePage", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mSubmitted{};
    ReflectProperty(mSubmitted, "Submitted", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
