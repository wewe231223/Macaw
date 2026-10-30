/**
 * @file Application.cpp
 * @brief 플랫폼, 렌더러와 에디터를 연결하는 애플리케이션 수명을 관리합니다.
 * @ingroup App
 */

#include "Application.h"
#include "AppReflection.h"
#include "CoreUObject/UObject.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Memory.h"
#include "Core/Stats.h"
#include "Editor/FProjectBrowser.h"

#include <array>

namespace App {

    /**
     * @brief FApplication의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    App::FApplication::FApplication() {
        App::ReflectionModules::RegisterAppTypes();
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    App::FApplication::~FApplication() {
        Shutdown();
    }

    /**
     * @brief 창과 렌더러·에디터를 초기화하고 종료 요청까지 프레임 루프를 실행합니다.
     */
    int App::FApplication::Run() {
        if (!mPlatform.Create(L"Macaw", 1440, 900)) {
            Core::ErrorHandler::Fatal("Application", "Window creation failed");
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};

        if (Length == 0 || Length >= Executable.size()) {
            Core::ErrorHandler::Fatal("Application", "Cannot locate the project templates");
        }

        mTemplateDirectory = std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project";

        if (!BrowseProjects({}, false)) {
            Shutdown();
            return 0;
        }

        while (mPlatform.PumpMessages() && !mEngine.WantsExit()) {
            if (mPlatform.IsMinimized()) {
                MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
                continue;
            }

            {
                const Core::FScopedStatFrame StatFrame{};

                mEngine.TickFrame(mTimer.Tick());
                mRenderer.Present();
            }

            if (mBrowseProjects) {
                const std::filesystem::path Requested{std::move(mPendingProject)};

                mPendingProject.clear();
                mBrowseProjects = false;
                mEngine.Shutdown();
                mRenderer.Shutdown();
                mEngine.SetProject(nullptr);
                mProject.reset();

                if (!BrowseProjects(Requested, mCreateProject)) {
                    break;
                }
            }
        }

        Shutdown();
        Core::ErrorHandler::Report(Core::FMemory::GetStats().mHeapBytes != 0, "Application", "UObject allocations remain after application shutdown", Core::ErrorHandler::EErrorLevel::Critical);

        return 0;
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void App::FApplication::Shutdown() {
        mEngine.Shutdown();
        mRenderer.Shutdown();
        mEngine.SetProject(nullptr);
        mProject.reset();
        mPlatform.Shutdown();
    }

    bool App::FApplication::BrowseProjects(std::filesystem::path Path, bool Create) {
        Editor::FProjectBrowser Browser{};
        Core::FString Error{};

        while (Browser.Show(mPlatform, mRenderer, Path, Create, Error)) {
            if (OpenProject(Path, Create, Error)) {
                return true;
            }
        }

        return false;
    }

    bool App::FApplication::OpenProject(const std::filesystem::path& Path, bool Create, Core::FString& Error) {
        auto Project{std::make_unique<Engine::FProject>()};

        if (!(Create ? Project->Create(Path, mTemplateDirectory, Error) : Project->Open(Path, Error))) {
            return false;
        }

        mEngine.Shutdown();
        mRenderer.Shutdown();
        mProject = std::move(Project);
        mEngine.SetProject(mProject.get());
        mRenderer.SetProjectPaths(mProject->GetAssets()->GetContentRoot(), mProject->GetShaderCachePath());

        if (!mRenderer.Initialize(mPlatform.GetWindowHandle(), mPlatform.GetWidth(), mPlatform.GetHeight()) || !mEngine.InitializeEditor(&mRenderer, &mPlatform)) {
            Core::ErrorHandler::Fatal("Project", "Cannot initialize the project editor");
        }

        mEngine.GetEditorContext()->SetProjectRequestHandler([this](const std::filesystem::path& Requested, bool CreateRequested) {
            mPendingProject = Requested;
            mCreateProject = CreateRequested;
            mBrowseProjects = true;
        });

        const std::u8string Name{reinterpret_cast<const char8_t*>(mProject->GetName().data()), mProject->GetName().size()};
        const std::wstring Title{L"Macaw - " + std::filesystem::path{Name}.wstring()};
        SetWindowTextW(mPlatform.GetWindowHandle(), Title.c_str());
        Editor::FProjectBrowser::RememberProject(mProject->GetFilePath());
        mTimer.Tick();

        return true;
    }

}
