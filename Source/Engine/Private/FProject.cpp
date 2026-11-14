/**
 * @file FProject.cpp
 * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리자에 대한 구현입니다.
 * @ingroup Engine
 */

#include "Engine/FProject.h"
#include "Engine/Cascade/FParticleSystemAssetManager.h"
#include "Core/FilePath.h"
#include "Serialization/JsonArchive.h"

#include <string>

namespace Engine {

    /**
     * @brief 프로젝트 설정 파일, 시작 장면과 에셋 관리를 담당하는 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 템플릿 상속 관계를 따라 적용할 원본 디렉터리들을 수집합니다.
         * @param Directory 검색하거나 로드할 템플릿 디렉터리입니다.
         * @param Layers 상속 순서로 적용할 템플릿 디렉터리 출력 목록입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @param Depth 탐색할 상위 보기의 깊이 또는 노드 배치 깊이입니다.
         * @return 상속 관계를 유효하게 해석했으면 true이며 실패 원인은 Error에 기록합니다.
         */
        bool CollectTemplateLayers(const std::filesystem::path& Directory, Core::TArray<std::filesystem::path>& Layers, Core::FString& Error, Core::Uint32 Depth) {
            if (Depth >= 8) {
                Error = "Project template inheritance is cyclic or too deep";

                return false;
            }

            std::error_code Code{};
            const auto Canonical{std::filesystem::canonical(Directory, Code)};

            if (Code || !std::filesystem::is_regular_file(Canonical / "Template.mproject", Code)) {
                Error = "Cannot find a project template: " + Core::FilePath::ToUtf8(Directory);

                return false;
            }

            const auto Manifest{Canonical / "Template.json"};

            if (std::filesystem::exists(Manifest, Code)) {
                Serialization::FJsonArchive Archive{Core::EArchiveMode::Loading};

                if (!Archive.Load(Manifest)) {
                    Error = Archive.GetError();

                    return false;
                }

                if (Archive.HasField("BaseTemplate")) {
                    Core::FString Base{};
                    Archive.Serialize("BaseTemplate", Base);
                    const std::filesystem::path BasePath{Core::FilePath::FromUtf8(Base)};

                    if (Archive.HasError() || Base.empty() || BasePath.has_root_path() || BasePath.has_parent_path() || BasePath == "." || BasePath == "..") {
                        Error = "Invalid base project template";

                        return false;
                    }

                    if (!CollectTemplateLayers(Canonical.parent_path() / BasePath, Layers, Error, Depth + 1)) {
                        return false;
                    }
                }
            }

            if (Code) {
                Error = "Cannot read project template metadata: " + Code.message();

                return false;
            }

            Layers.push_back(Canonical);

            return true;
        }
    }

    /**
     * @brief FProject의 초기 상태를 구성합니다.
     */
    Engine::FProject::FProject() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Engine::FProject::~FProject() = default;

    /**
     * @brief 현재 엔진이 지원하는 프로젝트 설정 파일의 버전을 반환합니다.
     */
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

        Core::TArray<std::filesystem::path> Layers{};

        if (!CollectTemplateLayers(TemplateDirectory, Layers, Error, 0)) {
            return false;
        }

        std::filesystem::create_directories(File.parent_path() / "Content", Code);

        for (const auto& Layer : Layers) {
            if (Code) {
                break;
            }

            std::filesystem::copy(Layer / "Content", File.parent_path() / "Content", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, Code);

            if (!Code && std::filesystem::exists(Layer / "Scripts", Code)) {
                std::filesystem::create_directories(File.parent_path() / "Scripts", Code);

                if (!Code) {
                    std::filesystem::copy(Layer / "Scripts", File.parent_path() / "Scripts", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, Code);
                }
            }

            for (const char* Name : {"README.md", ".gitignore"}) {
                if (!Code && std::filesystem::exists(Layer / Name, Code)) {
                    std::filesystem::copy_file(Layer / Name, File.parent_path() / Name, std::filesystem::copy_options::overwrite_existing, Code);
                }
            }
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

        auto ParticleSystems{std::make_unique<Engine::FParticleSystemAssetManager>(*Assets)};

        mVersion = Version;
        mFilePath = File;
        mName = std::move(Name);
        mStartupScene = RelativePath;
        mParticleSystems.reset();
        mAssets = std::move(Assets);
        mParticleSystems = std::move(ParticleSystems);

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

    /**
     * @brief 마지막으로 연 프로젝트 설정의 버전을 반환합니다.
     */
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

    Engine::FParticleSystemAssetManager* Engine::FProject::GetParticleSystems() const {
        return mParticleSystems.get();
    }

}
