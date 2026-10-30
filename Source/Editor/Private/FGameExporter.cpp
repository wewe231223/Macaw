/**
 * @file FGameExporter.cpp
 * @brief 프로젝트를 독립 실행 게임 폴더로 내보내는 기능을 구현합니다.
 * @ingroup Editor
 */

#include "Editor/FGameExporter.h"
#include "Core/FilePath.h"
#include "Engine/FProject.h"
#include "Engine/FWorldContext.h"
#include "Engine/SceneSerializer.h"
#include "Engine/UWorld.h"

namespace Editor {

    /**
     * @brief 이 구현 파일에서만 사용하는 파일 복사 보조 함수입니다.
     */
    namespace {

        /**
         * @brief 대상 폴더를 만들고 파일을 덮어써서 복사합니다.
         * @param[in] Source 복사할 원본 파일입니다.
         * @param[in] Destination 복사할 대상 파일 경로입니다.
         * @note 원본과 대상이 같은 파일이면 아무것도 하지 않습니다. 실패하면 std::filesystem::filesystem_error를 던집니다.
         */
        void CopyFile(const std::filesystem::path& Source, const std::filesystem::path& Destination) {
            if (std::filesystem::exists(Destination) && std::filesystem::equivalent(Source, Destination)) {
                return;
            }

            std::filesystem::create_directories(Destination.parent_path());
            std::filesystem::copy_file(Source, Destination, std::filesystem::copy_options::overwrite_existing);
        }

        /**
         * @brief 폴더 구조를 유지하며 하위 파일을 모두 복사합니다.
         * @param[in] Source 복사할 원본 폴더입니다.
         * @param[in] Destination 복사할 대상 폴더입니다.
         * @note 순회 중 대상이 원본 안에 있어도 영향을 받지 않도록 원본 목록을 먼저 수집한 뒤 복사합니다.
         */
        void CopyDirectory(const std::filesystem::path& Source, const std::filesystem::path& Destination) {
            Core::TArray<std::filesystem::directory_entry> Entries{};

            for (const auto& Entry : std::filesystem::recursive_directory_iterator{Source}) {
                Entries.push_back(Entry);
            }

            std::filesystem::create_directories(Destination);

            for (const auto& Entry : Entries) {
                const auto Target{Destination / Entry.path().lexically_relative(Source)};

                if (Entry.is_directory()) {
                    std::filesystem::create_directories(Target);
                } else if (Entry.is_regular_file()) {
                    CopyFile(Entry.path(), Target);
                }
            }
        }

    }

    /**
     * @brief 시작 장면을 검증한 뒤 게임 런타임과 프로젝트 파일을 내보낼 폴더에 복사합니다.
     */
    bool Editor::FGameExporter::Export(const Engine::FProject& Project, const std::filesystem::path& RuntimeDirectory, const std::filesystem::path& Destination, Core::FString& Error) {
        Error.clear();

        if (Project.GetAssets() == nullptr || Destination.empty()) {
            Error = "Open a project and choose an export folder";
            return false;
        }

        try {
            const auto Runtime{std::filesystem::absolute(RuntimeDirectory).lexically_normal()};
            const auto Output{std::filesystem::absolute(Destination).lexically_normal()};

            if (!std::filesystem::is_regular_file(Runtime / "MacawGame.exe")) {
                Error = "The game executable is missing. Build Macaw before exporting.";
                return false;
            }

            const auto ScenePath{Project.GetAssets()->ResolvePath(Core::FilePath::FromUtf8(Project.GetStartupScene()), Error)};
            const auto Scene{ScenePath.empty() ? nullptr : Engine::FSceneSerializer::Load(ScenePath, Error, Project.GetAssets(), Engine::EWorldType::Game)};

            if (Scene == nullptr) {
                return false;
            }

            if (Scene->GetWorld()->GetActiveCamera() == nullptr) {
                Error = "The startup scene requires an active world camera. Set one in the editor and save the scene.";
                return false;
            }

            Core::TArray<std::filesystem::path> Libraries{};

            for (const auto& Entry : std::filesystem::directory_iterator{Runtime}) {
                if (Entry.is_regular_file() && _wcsicmp(Entry.path().extension().c_str(), L".dll") == 0) {
                    Libraries.push_back(Entry.path());
                }
            }

            CopyDirectory(Project.GetAssets()->GetContentRoot(), Output / "Content");
            CopyDirectory(Runtime / "Templates", Output / "Templates");
            CopyFile(Project.GetFilePath(), Output / "Game.mproject");

            std::filesystem::path ExecutableName{Project.GetFilePath().filename()};
            ExecutableName.replace_extension(L".exe");
            CopyFile(Runtime / "MacawGame.exe", Output / ExecutableName);

            for (const auto& Library : Libraries) {
                CopyFile(Library, Output / Library.filename());
            }

            return true;
        } catch (const std::filesystem::filesystem_error& Exception) {
            Error = "Cannot export the game: " + Core::FString{Exception.what()};
            return false;
        }
    }

}
