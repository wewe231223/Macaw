/**
 * @file FProject.cpp
 * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리자에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FProject.h"
#include "Core/FilePath.h"
#include "Serialization/JsonArchive.h"

FProject::FProject() = default;

FProject::~FProject() = default;

bool FProject::Create(const std::filesystem::path& ProjectFile, const std::filesystem::path& TemplateDirectory, FString& Error) {
    Error.clear();
    std::error_code Code{};
    const auto File{std::filesystem::absolute(ProjectFile, Code).lexically_normal()};
    if (Code || File.extension() != L".mproject" || File.stem().empty() || (std::filesystem::exists(File.parent_path(), Code) && !std::filesystem::is_empty(File.parent_path(), Code)) || Code) {
        Error = "Create a .mproject file in a new or empty project folder";
        return false;
    }
    FProject Candidate{};
    if (!Candidate.Open(TemplateDirectory / "Template.mproject", Error)) {
        return false;
    }
    std::filesystem::create_directories(File.parent_path() / "Content", Code);
    if (!Code) {
        std::filesystem::copy(TemplateDirectory / "Content", File.parent_path() / "Content", std::filesystem::copy_options::recursive, Code);
    }
    for (const char* Folder : {"Maps", "Meshes", "Textures", "Materials", "Scripts"}) {
        if (!Code) {
            std::filesystem::create_directories(File.parent_path() / "Content" / Folder, Code);
        }
    }
    if (Code) {
        Error = "Cannot create project content: " + Code.message();
        return false;
    }

    Candidate.mFilePath = File;
    Candidate.mName = FilePath::ToUtf8(File.stem());
    Candidate.mAssets = std::make_unique<FAssetManager>(File.parent_path() / "Content");
    if (!Candidate.Save(Error)) {
        return false;
    }
    return Open(File, Error);
}

bool FProject::Open(const std::filesystem::path& ProjectFile, FString& Error) {
    Error.clear();
    std::error_code Code{};
    const auto File{std::filesystem::canonical(ProjectFile, Code)};
    if (Code || File.extension() != L".mproject") {
        Error = "Cannot open the project file";
        return false;
    }
    FJsonArchive Archive{EArchiveMode::Loading};
    if (!Archive.Load(File)) {
        Error = Archive.GetError();
        return false;
    }
    Uint32 Version{};
    FString Name{};
    FString StartupScene{};
    Archive.Serialize("Version", Version);
    Archive.Serialize("Name", Name);
    Archive.Serialize("StartupScene", StartupScene);
    if (Archive.HasError() || Version != 1 || Name.empty() || Name.find('\0') != FString::npos || StartupScene.empty() || StartupScene.find('\0') != FString::npos) {
        Error = Archive.HasError() ? Archive.GetError() : "Unsupported or invalid project settings";
        return false;
    }
    auto Assets{std::make_unique<FAssetManager>(File.parent_path() / "Content")};
    const std::filesystem::path RelativeScene{FilePath::FromUtf8(StartupScene)};
    if (RelativeScene.is_absolute() || RelativeScene.has_root_name()) {
        Error = "The startup scene must use a Content relative path";
        return false;
    }
    const FString RelativePath{Assets->GetRelativePath(RelativeScene, Error)};
    if (RelativePath.empty()) {
        return false;
    }
    mFilePath = File;
    mName = std::move(Name);
    mStartupScene = RelativePath;
    mAssets = std::move(Assets);
    return true;
}

bool FProject::Save(FString& Error) const {
    Error.clear();
    if (mFilePath.empty() || mAssets == nullptr) {
        Error = "No project is open";
        return false;
    }
    FJsonArchive Archive{EArchiveMode::Saving};
    Uint32 Version{1};
    FString Name{mName};
    FString StartupScene{mStartupScene};
    Archive.Serialize("Version", Version);
    Archive.Serialize("Name", Name);
    Archive.Serialize("StartupScene", StartupScene);
    const bool Saved{!Archive.HasError() && Archive.Save(mFilePath)};
    Error = Archive.GetError();
    return Saved;
}

bool FProject::SetStartupScene(const std::filesystem::path& Path, FString& Error) {
    if (mAssets == nullptr) {
        Error = "No project is open";
        return false;
    }
    const FString Relative{mAssets->GetRelativePath(Path, Error)};
    if (Relative.empty()) {
        return false;
    }
    const FString Previous{mStartupScene};
    mStartupScene = Relative;
    if (!Save(Error)) {
        mStartupScene = Previous;
        return false;
    }
    return true;
}

const FString& FProject::GetName() const {
    return mName;
}

const FString& FProject::GetStartupScene() const {
    return mStartupScene;
}

const std::filesystem::path& FProject::GetFilePath() const {
    return mFilePath;
}

std::filesystem::path FProject::GetRootPath() const {
    return mFilePath.parent_path();
}

std::filesystem::path FProject::GetShaderCachePath() const {
    return GetRootPath() / "Intermediate/ShaderCache";
}

FAssetManager* FProject::GetAssets() const {
    return mAssets.get();
}
