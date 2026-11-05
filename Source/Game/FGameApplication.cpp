/**
 * @file FGameApplication.cpp
 * @brief 내보낸 프로젝트의 시작 장면을 에디터 없이 실행하는 게임 애플리케이션입니다.
 * @ingroup Game
 */

#include "FGameApplication.h"
#include "GameReflection.h"
#include "ScriptReflection.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "Core/Stats.h"
#include "Engine/FWorldContext.h"
#include "Engine/SceneSerializer.h"
#include "Engine/UWorld.h"
#include "Script/FScriptRuntime.h"

#include <algorithm>

namespace Game {

    /**
     * @brief Game과 Script 모듈의 리플렉션 타입을 등록합니다.
     */
    Game::FGameApplication::FGameApplication() {
        Game::ReflectionModules::RegisterGameTypes();
        Script::ReflectionModules::RegisterScriptTypes();
    }

    /**
     * @brief 메시지 처리기, 뷰포트, 엔진, 렌더러와 창을 정리합니다.
     */
    Game::FGameApplication::~FGameApplication() {
        Shutdown();
    }

    /**
     * @brief 프로젝트를 초기화하고 창이 닫히거나 월드가 정지할 때까지 프레임 루프를 실행합니다.
     */
    int Game::FGameApplication::Run(const std::filesystem::path& ProjectFile) {
        Core::FString Error{};

        if (!Initialize(ProjectFile, Error)) {
            return ReportFailure(Error);
        }

        mTimer.Tick();

        while (mPlatform.PumpMessages()) {
            if (mPlatform.IsMinimized()) {
                mPlatform.GetInput().SetGameInputEnabled(false);
                mTimer.Tick();
                MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
                continue;
            }

            const Core::FScopedStatFrame StatFrame{};
            const float DeltaTime{std::min(mTimer.Tick(), 0.1f)};
            mRenderer.BeginFrame(mPlatform.GetWidth(), mPlatform.GetHeight());

            if (!mViewport->Resize(mPlatform.GetWidth(), mPlatform.GetHeight())) {
                return ReportFailure("Cannot resize the game viewport");
            }

            mPlatform.GetInput().SetGameInputEnabled(true);

            if (mEngine.GetWorld()->GetPlayState() == Engine::EWorldPlayState::Playing) {
                mGameClient.ProcessInput(mPlatform.GetInput(), DeltaTime);
            }

            mEngine.Tick(DeltaTime, false);

            Engine::UWorld* World{mEngine.GetWorld()};

            if (World == nullptr || World->GetActiveCamera() == nullptr) {
                return ReportFailure("The game world has no active camera");
            }

            if (World->GetPlayState() == Engine::EWorldPlayState::Stopped) {
                break;
            }

            mEngine.SynchronizeWorld();
            mViewport->Draw();

            if (!mRenderer.CopyToBackBuffer(mViewport->GetRenderTarget())) {
                return ReportFailure("Cannot display the game viewport");
            }

            mRenderer.Present();
        }

        Shutdown();

        return 0;
    }

    /**
     * @brief 프로젝트를 열고 창, 렌더러, 엔진을 초기화한 뒤 시작 장면으로 게임 월드를 시작합니다.
     */
    bool Game::FGameApplication::Initialize(const std::filesystem::path& ProjectFile, Core::FString& Error) {
        if (!mProject.Open(ProjectFile, Error)) {
            return false;
        }

        const std::wstring Title{Core::FilePath::FromUtf8(mProject.GetName()).wstring()};

        if (!mPlatform.Create(Title.c_str(), 1280, 720)) {
            Error = "Cannot create the game window";
            return false;
        }

        mRenderer.SetProjectPaths(mProject.GetAssets()->GetContentRoot(), mProject.GetRootPath() / "Saved/ShaderCache");
        mEngine.SetProject(&mProject);

        if (!mRenderer.Initialize(mPlatform.GetWindowHandle(), mPlatform.GetWidth(), mPlatform.GetHeight()) || !mEngine.Initialize(&mRenderer)) {
            Error = "Cannot initialize the game renderer or engine";
            return false;
        }

        const auto ScenePath{mProject.GetAssets()->ResolvePath(Core::FilePath::FromUtf8(mProject.GetStartupScene()), Error)};
        auto Context{ScenePath.empty() ? nullptr : Engine::FSceneSerializer::Load(ScenePath, Error, mProject.GetAssets(), Engine::EWorldType::Game)};

        if (Context == nullptr) {
            return false;
        }

        if (Context->GetWorld()->GetActiveCamera() == nullptr) {
            Error = "The startup scene requires an active world camera. Set one in the editor and save the scene.";
            return false;
        }

        Script::FScriptRuntime::Get()->BeginSession(mProject.GetRootPath());

        if (!mEngine.ReplaceWorld(std::move(Context)) || !mGameClient.SetWorld(mEngine.GetWorldHandle()) || !mEngine.GetWorld()->BeginPlay()) {
            Error = "Cannot start the game world";
            return false;
        }

        mViewport = std::make_unique<Engine::FSceneViewport>(&mRenderer, &mGameClient);

        return true;
    }

    /**
     * @brief 오류를 기록하고 오류 대화상자를 표시합니다.
     */
    int Game::FGameApplication::ReportFailure(const Core::FString& Error) const {
        Core::ErrorHandler::Report("Game", Error, Core::ErrorHandler::EErrorLevel::Error);

        MessageBoxW(mPlatform.GetWindowHandle(), Core::FilePath::FromUtf8(Error).c_str(), L"Cannot Run Game", MB_OK | MB_ICONERROR);

        return 1;
    }

    /**
     * @brief 메시지 처리기, 뷰포트, 엔진, 렌더러와 창을 역순으로 정리합니다.
     */
    void Game::FGameApplication::Shutdown() {
        mPlatform.GetInput().SetGameInputEnabled(false);
        mViewport.reset();
        mGameClient.SetWorld({});
        mEngine.Shutdown();
        Script::FScriptRuntime::Get()->EndSession();
        mEngine.SetProject(nullptr);
        mRenderer.Shutdown();
        mPlatform.Shutdown();
    }

}
