/**
 * @file FProject.cpp
 * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리자에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FProject.h"
#include "Core/FilePath.h"
#include "Serialization/JsonArchive.h"

#include <string>

namespace Engine {

    Engine::FProject::FProject() = default;

    Engine::FProject::~FProject() = default;

    Core::Uint32 Engine::FProject::GetCurrentVersion() {
        return 2;
    }

    /**
     * @brief 빈 프로젝트 폴더에 템플릿 콘텐츠와 초기 장면 및 설정 파일을 생성합니다.
     */
    bool Engine::FProject::Create(const std::filesystem::path& ProjectFile, const std::filesystem::path& TemplateDirectory, Core::FString& Error) {
        Error.clear();

        std::error_code Code{};
        const auto File{std::filesystem::absolute(ProjectFile, Code).lexically_normal()};

        if (Code || File.extension() != L".mproject" || File.stem().empty() || (std::filesystem::exists(File.parent_path(), Code) && !std::filesystem::is_empty(File.parent_path(), Code)) || Code) {
            Error = "Create a .mproject file in a new or empty project folder";

            return false;
        }

        Engine::FProject Candidate{};

        if (!Candidate.Open(TemplateDirectory / "Template.mproject", Error)) {
            return false;
        }

        std::filesystem::create_directories(File.parent_path() / "Content", Code);

        if (!Code) {
            std::filesystem::copy(TemplateDirectory / "Content", File.parent_path() / "Content", std::filesystem::copy_options::recursive, Code);
        }

        // C# 스크립트는 Content 밖의 Scripts 폴더에 두므로 Content에는 스크립트 폴더를 만들지 않습니다.
        for (const char* Folder : {"Maps", "Meshes", "Textures", "Materials"}) {
            if (!Code) {
                std::filesystem::create_directories(File.parent_path() / "Content" / Folder, Code);
            }
        }

        if (Code) {
            Error = "Cannot create project content: " + Code.message();

            return false;
        }

        Candidate.mFilePath = File;
        Candidate.mName = Core::FilePath::ToUtf8(File.stem());
        Candidate.mAssets = std::make_unique<Asset::FAssetManager>(File.parent_path() / "Content");

        if (!Candidate.Save(Error)) {
            return false;
        }

        return Open(File, Error);
    }

    /**
     * @brief 프로젝트 설정의 버전과 콘텐츠 경로를 검증하여 현재 프로젝트로 엽니다.
     */
    bool Engine::FProject::Open(const std::filesystem::path& ProjectFile, Core::FString& Error) {
        Error.clear();

        std::error_code Code{};
        const auto File{std::filesystem::canonical(ProjectFile, Code)};

        if (Code || File.extension() != L".mproject") {
            Error = "Cannot open the project file";

            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

        if (!Archive.Load(File)) {
            Error = Archive.GetError();

            return false;
        }

        Core::Uint32 Version{};
        Core::FString Name{};
        Core::FString StartupScene{};

        Archive.Serialize("Version", Version);

        if (Archive.HasError()) {
            Error = Archive.GetError();

            return false;
        }

        const Core::Uint32 CurrentVersion{GetCurrentVersion()};

        if (Version != CurrentVersion) {
            Error = "Unsupported project version " + std::to_string(Version) + "; expected " + std::to_string(CurrentVersion);

            return false;
        }

        Archive.Serialize("Name", Name);
        Archive.Serialize("StartupScene", StartupScene);

        if (Archive.HasError() || Name.empty() || Name.find('\0') != Core::FString::npos || StartupScene.empty() || StartupScene.find('\0') != Core::FString::npos) {
            Error = Archive.HasError() ? Archive.GetError() : "Unsupported or invalid project settings";

            return false;
        }

        auto Assets{std::make_unique<Asset::FAssetManager>(File.parent_path() / "Content")};
        const std::filesystem::path RelativeScene{Core::FilePath::FromUtf8(StartupScene)};

        if (RelativeScene.is_absolute() || RelativeScene.has_root_name()) {
            Error = "The startup scene must use a Content relative path";

            return false;
        }

        const Core::FString RelativePath{Assets->GetRelativePath(RelativeScene, Error)};

        if (RelativePath.empty()) {
            return false;
        }

        mVersion = Version;
        mFilePath = File;
        mName = std::move(Name);
        mStartupScene = RelativePath;
        mAssets = std::move(Assets);

        return true;
    }

    /**
     * @brief 프로젝트 이름과 시작 장면 경로를 설정 파일에 저장합니다.
     */
    bool Engine::FProject::Save(Core::FString& Error) const {
        Error.clear();

        if (mFilePath.empty() || mAssets == nullptr) {
            Error = "No project is open";

            return false;
        }

        Serialization::FJsonArchive Archive{Core::EArchiveMode::Saving};
        Core::Uint32 Version{mVersion};
        Core::FString Name{mName};
        Core::FString StartupScene{mStartupScene};

        Archive.Serialize("Version", Version);
        Archive.Serialize("Name", Name);
        Archive.Serialize("StartupScene", StartupScene);

        const bool Saved{!Archive.HasError() && Archive.Save(mFilePath)};

        Error = Archive.GetError();

        return Saved;
    }

    /**
     * @brief 콘텐츠 경로를 검증하여 시작 장면을 변경하고 프로젝트 설정을 저장합니다.
     */
    bool Engine::FProject::SetStartupScene(const std::filesystem::path& Path, Core::FString& Error) {
        if (mAssets == nullptr) {
            Error = "No project is open";

            return false;
        }

        const Core::FString Relative{mAssets->GetRelativePath(Path, Error)};

        if (Relative.empty()) {
            return false;
        }

        const Core::FString Previous{mStartupScene};

        mStartupScene = Relative;

        if (!Save(Error)) {
            mStartupScene = Previous;

            return false;
        }

        return true;
    }

    Core::Uint32 Engine::FProject::GetVersion() const {
        return mVersion;
    }

    /**
     * @brief 프로젝트의 표시 이름을 반환합니다.
     */
    const Core::FString& Engine::FProject::GetName() const {
        return mName;
    }

    /**
     * @brief 프로젝트 콘텐츠 폴더를 기준으로 한 시작 장면 경로를 반환합니다.
     */
    const Core::FString& Engine::FProject::GetStartupScene() const {
        return mStartupScene;
    }

    /**
     * @brief 현재 프로젝트 설정 파일의 절대 경로를 반환합니다.
     */
    const std::filesystem::path& Engine::FProject::GetFilePath() const {
        return mFilePath;
    }

    /**
     * @brief 프로젝트 설정 파일을 포함하는 루트 폴더를 반환합니다.
     */
    std::filesystem::path Engine::FProject::GetRootPath() const {
        return mFilePath.parent_path();
    }

    /**
     * @brief 현재 프로젝트에서 사용할 셰이더 캐시 폴더를 반환합니다.
     */
    std::filesystem::path Engine::FProject::GetShaderCachePath() const {
        return GetRootPath() / "Intermediate/ShaderCache";
    }

    /**
     * @brief 프로젝트의 에셋 관리자를 비소유 주소로 반환합니다.
     */
    Asset::FAssetManager* Engine::FProject::GetAssets() const {
        return mAssets.get();
    }

}
