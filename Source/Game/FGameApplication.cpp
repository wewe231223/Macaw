/**
 * @file FGameApplication.cpp
 * @brief 내보낸 프로젝트의 시작 장면을 에디터 없이 실행하는 게임 애플리케이션입니다.
 * @ingroup Game
 */

#include "FGameApplication.h"
#include "GameReflection.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "Core/Stats.h"
#include "Engine/FWorldContext.h"
#include "Engine/SceneSerializer.h"
#include "Engine/UWorld.h"

#include <algorithm>

namespace Game {

    /**
     * @brief Game 모듈의 리플렉션 타입을 등록합니다.
     */
    Game::FGameApplication::FGameApplication() {
        Game::ReflectionModules::RegisterGameTypes();
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
                mMouseReady = false;
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

            mViewport->Input(ReadInput(), DeltaTime);
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

        if (!mEngine.ReplaceWorld(std::move(Context)) || !mGameClient.SetWorld(mEngine.GetWorldHandle()) || !mEngine.GetWorld()->BeginPlay()) {
            Error = "Cannot start the game world";
            return false;
        }

        mViewport = std::make_unique<Engine::FSceneViewport>(&mRenderer, &mGameClient);
        mPlatform.SetMessageHandler([this](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            return HandleMessage(Window, Message, WParam, LParam);
        });

        return true;
    }

    /**
     * @brief 현재 마우스·키보드 상태를 읽어 이번 프레임의 뷰포트 입력을 만듭니다.
     */
    Engine::FViewportInput Game::FGameApplication::ReadInput() {
        Engine::FViewportInput Input{};
        Input.mFocused = GetForegroundWindow() == mPlatform.GetWindowHandle();

        POINT Mouse{};
        const bool HasMouse{GetCursorPos(&Mouse) != FALSE && ScreenToClient(mPlatform.GetWindowHandle(), &Mouse) != FALSE};
        Input.mMouse = {static_cast<float>(Mouse.x), static_cast<float>(Mouse.y)};
        Input.mHovered = HasMouse && Mouse.x >= 0 && Mouse.y >= 0 && Mouse.x < static_cast<LONG>(mPlatform.GetWidth()) && Mouse.y < static_cast<LONG>(mPlatform.GetHeight());

        if (Input.mFocused && HasMouse) {
            Input.mDelta = mMouseReady ? Input.mMouse - mPreviousMouse : Core::FVector2{};
            Input.mLeftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            Input.mRightDown = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
            Input.mMiddleDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
            const bool LeftDown{Input.mLeftDown};
            Input.mLeftPressed = LeftDown && !mLeftDown;
            Input.mLeftReleased = !LeftDown && mLeftDown;
            Input.mMiddlePressed = Input.mMiddleDown && !mMiddleDown;
            Input.mMiddleReleased = !Input.mMiddleDown && mMiddleDown;
            Input.mForward = (GetAsyncKeyState('W') & 0x8000) != 0;
            Input.mBackward = (GetAsyncKeyState('S') & 0x8000) != 0;
            Input.mLeft = (GetAsyncKeyState('A') & 0x8000) != 0;
            Input.mRight = (GetAsyncKeyState('D') & 0x8000) != 0;
            Input.mUp = (GetAsyncKeyState('E') & 0x8000) != 0;
            Input.mDown = (GetAsyncKeyState('Q') & 0x8000) != 0;
            Input.mFast = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
            Input.mAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            Input.mControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            Input.mEscape = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
            Input.mWheel = mWheel;
            mLeftDown = LeftDown;
            mMiddleDown = Input.mMiddleDown;
        } else {
            mLeftDown = false;
            mMiddleDown = false;
        }

        mMouseReady = Input.mFocused && HasMouse;
        mPreviousMouse = Input.mMouse;
        mWheel = 0.0f;

        return Input;
    }

    /**
     * @brief 마우스 휠을 누적하고 버튼 입력 동안 마우스 캡처를 관리합니다.
     */
    bool Game::FGameApplication::HandleMessage(HWND Window, UINT Message, WPARAM WParam, LPARAM) {
        if (Message == WM_MOUSEWHEEL) {
            mWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(WParam)) / static_cast<float>(WHEEL_DELTA);
            return true;
        }

        if (Message == WM_LBUTTONDOWN || Message == WM_RBUTTONDOWN || Message == WM_MBUTTONDOWN) {
            SetCapture(Window);
        } else if ((Message == WM_LBUTTONUP || Message == WM_RBUTTONUP || Message == WM_MBUTTONUP) && (WParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON)) == 0) {
            ReleaseCapture();
        } else if (Message == WM_KILLFOCUS) {
            ReleaseCapture();
            mMouseReady = false;
            mWheel = 0.0f;
        }

        return false;
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
        mPlatform.SetMessageHandler({});
        mViewport.reset();
        mGameClient.SetWorld({});
        mEngine.Shutdown();
        mEngine.SetProject(nullptr);
        mRenderer.Shutdown();
        mPlatform.Shutdown();
    }

}
