#include "Platform/SplashWindow.h"
#include "Platform/WindowResources.h"
#include "Core/Base/ErrorHandler.h"
#include "FSplashCanvas.h"

#include <dwmapi.h>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace Platform {

    namespace {
        constexpr Core::Int32 LogicalWidth{640};
        constexpr Core::Int32 LogicalHeight{360};
        constexpr DWORD FrameMilliseconds{16};
        constexpr float FinishTimeout{0.4f};
        constexpr wchar_t ClassName[]{L"MacawSplashWindow"};

        std::wstring Widen(std::string_view Text) {
            if (Text.empty()) {
                return {};
            }

            const Core::Int32 Length{MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<Core::Int32>(Text.size()), nullptr, 0)};
            std::wstring Result(static_cast<std::size_t>(std::max(Length, 0)), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<Core::Int32>(Text.size()), Result.data(), Length);

            return Result;
        }

        LRESULT CALLBACK SplashProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            switch (Message) {
            case WM_ERASEBKGND:
                return 1;
            case WM_CLOSE:
                return 0;
            case WM_NCHITTEST:
                return HTCAPTION;
            default:
                return DefWindowProcW(Window, Message, WParam, LParam);
            }
        }
    }

    Platform::FSplashWindow::FSplashWindow() = default;

    Platform::FSplashWindow::~FSplashWindow() {
        Close();
    }

    void Platform::FSplashWindow::Show(HWND Anchor, std::wstring_view ProjectName, const std::filesystem::path& ProjectDirectory) {
        if (mThread.joinable()) {
            return;
        }

        mProjectName = ProjectName;
        mProjectDirectory = ProjectDirectory;
        mStage = L"Starting...";
        mAction.clear();
        mFile.clear();
        mProgress = 0.0f;
        mStarted = false;
        mFinishRequested = false;
        mFinished = false;
        mCloseRequested = false;

        const HMONITOR Monitor{MonitorFromWindow(Anchor, MONITOR_DEFAULTTOPRIMARY)};
        mThread = std::thread{[this, Monitor]() {
            ThreadMain(Monitor);
        }};

        std::unique_lock Lock{mMutex};
        mSignal.wait(Lock, [this]() {
            return mStarted;
        });
    }

    void Platform::FSplashWindow::Finish() {
        if (!mThread.joinable()) {
            return;
        }

        std::unique_lock Lock{mMutex};
        mFinishRequested = true;
        mProgress = 1.0f;
        mSignal.wait(Lock, [this]() {
            return mFinished;
        });
    }

    void Platform::FSplashWindow::Close() {
        if (!mThread.joinable()) {
            return;
        }

        {
            const std::lock_guard Lock{mMutex};
            mCloseRequested = true;
        }

        mThread.join();
    }

    void Platform::FSplashWindow::OnStage(std::string_view Stage, float Progress) {
        std::wstring Text{Widen(Stage)};
        const std::lock_guard Lock{mMutex};

        mStage = std::move(Text);
        mAction.clear();
        mFile.clear();
        mProgress = std::max(mProgress, std::clamp(Progress, 0.0f, 1.0f));
    }

    void Platform::FSplashWindow::OnFile(std::string_view Action, const std::filesystem::path& File) {
        std::wstring Text{Widen(Action)};
        std::filesystem::path Display{File.lexically_normal()};

        if (!mProjectDirectory.empty()) {
            const std::filesystem::path Relative{Display.lexically_relative(mProjectDirectory)};

            if (!Relative.empty() && *Relative.begin() != L"..") {
                Display = Relative;
            }
        }

        std::wstring Path{Display.generic_wstring()};
        const std::lock_guard Lock{mMutex};

        mAction = std::move(Text);
        mFile = std::move(Path);
    }

    void Platform::FSplashWindow::ThreadMain(HMONITOR Monitor) {
        const HINSTANCE Instance{GetModuleHandleW(nullptr)};

        WNDCLASSEXW Class{};
        Class.cbSize = sizeof(Class);
        Class.style = CS_DROPSHADOW;
        Class.lpfnWndProc = &SplashProcedure;
        Class.hInstance = Instance;
        Class.hIcon = static_cast<HICON>(LoadImageW(Instance, MAKEINTRESOURCEW(MacawIconResource), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
        Class.hIconSm = static_cast<HICON>(LoadImageW(Instance, MAKEINTRESOURCEW(MacawIconResource), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
        Class.hCursor = LoadCursorW(nullptr, IDC_APPSTARTING);
        Class.lpszClassName = ClassName;

        if (RegisterClassExW(&Class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Core::ErrorHandler::ReportWin32(GetLastError(), "Splash", "Splash window class registration failed", Core::ErrorHandler::EErrorLevel::Warning);
        }

        MONITORINFO Info{};
        Info.cbSize = sizeof(Info);
        GetMonitorInfoW(Monitor, &Info);

        const LONG CenterX{(Info.rcWork.left + Info.rcWork.right) / 2};
        const LONG CenterY{(Info.rcWork.top + Info.rcWork.bottom) / 2};
        const HWND Window{CreateWindowExW(WS_EX_APPWINDOW, ClassName, L"Macaw", WS_POPUP, CenterX, CenterY, 1, 1, nullptr, nullptr, Instance, nullptr)};
        FSplashCanvas Canvas{};
        bool Ready{};

        if (Window != nullptr) {
            const float Scale{static_cast<float>(GetDpiForWindow(Window)) / 96.0f};
            const Core::Int32 Width{static_cast<Core::Int32>(std::lround(LogicalWidth * Scale))};
            const Core::Int32 Height{static_cast<Core::Int32>(std::lround(LogicalHeight * Scale))};
            const DWM_WINDOW_CORNER_PREFERENCE Corners{DWMWCP_ROUND};

            DwmSetWindowAttribute(Window, DWMWA_WINDOW_CORNER_PREFERENCE, &Corners, sizeof(Corners));
            SetWindowPos(Window, nullptr, CenterX - Width / 2, CenterY - Height / 2, Width, Height, SWP_NOZORDER | SWP_NOACTIVATE);
            Ready = Canvas.Initialize(Width, Height, Scale, mProjectName);
        }

        if (!Ready) {
            Core::ErrorHandler::Report("Splash", "Cannot create the splash window", Core::ErrorHandler::EErrorLevel::Warning);
        }

        const auto Start{std::chrono::steady_clock::now()};
        auto Previous{Start};
        float Displayed{};
        float FinishElapsed{};
        bool Shown{};

        while (Ready) {
            MSG Message{};

            while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&Message);
                DispatchMessageW(&Message);
            }

            std::wstring Stage{};
            std::wstring Action{};
            std::wstring File{};
            float Target{};
            bool Finishing{};

            {
                const std::lock_guard Lock{mMutex};

                if (mCloseRequested) {
                    break;
                }

                Stage = mStage;
                Action = mAction;
                File = mFile;
                Target = mProgress;
                Finishing = mFinishRequested && !mFinished;
            }

            const auto Now{std::chrono::steady_clock::now()};
            const float Delta{std::chrono::duration<float>(Now - Previous).count()};
            Previous = Now;

            Displayed += (Target - Displayed) * (1.0f - std::exp(-Delta * (Finishing ? 14.0f : 5.0f)));

            if (Finishing) {
                FinishElapsed += Delta;

                if (Displayed > 0.995f || FinishElapsed > FinishTimeout) {
                    Displayed = 1.0f;
                    const std::lock_guard Lock{mMutex};
                    mFinished = true;
                    mSignal.notify_all();
                }
            }

            if (!Shown) {
                ShowWindow(Window, SW_SHOWNORMAL);
            }

            const HDC Surface{GetDC(Window)};
            Canvas.Draw(Surface, Stage, Action, File, Displayed, std::chrono::duration<float>(Now - Start).count());
            ReleaseDC(Window, Surface);
            ValidateRect(Window, nullptr);

            if (!Shown) {
                Shown = true;
                const std::lock_guard Lock{mMutex};
                mStarted = true;
                mSignal.notify_all();
            }

            MsgWaitForMultipleObjects(0, nullptr, false, FrameMilliseconds, QS_ALLINPUT);
        }

        if (Window != nullptr) {
            DestroyWindow(Window);
        }

        const std::lock_guard Lock{mMutex};
        mStarted = true;
        mFinished = true;
        mSignal.notify_all();
    }

}
