/**
 * @file Window.cpp
 * @brief Win32 창과 메시지 루프를 관리합니다.
 * @ingroup Platform
 */

#include "Platform/Window.h"
#include "Platform/WindowResources.h"
#include "Core/Base/ErrorHandler.h"

#include <utility>

namespace Platform {

    /**
     * @brief FWindowsApplication의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Platform::FWindowsApplication::FWindowsApplication() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Platform::FWindowsApplication::~FWindowsApplication() {
        Shutdown();
    }

    /**
     * @brief 지정한 제목과 클라이언트 크기로 Win32 창을 생성합니다.
     */
    bool Platform::FWindowsApplication::Create(const wchar_t* Title, Core::Uint32 Width, Core::Uint32 Height, bool Visible) {
        if (mWindow != nullptr) {
            Core::ErrorHandler::Fatal("Window", "The application window already exists");
        }

        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        const HINSTANCE Instance{GetModuleHandleW(nullptr)};

        WNDCLASSEXW Class{};
        Class.cbSize = sizeof(Class);
        Class.style = CS_HREDRAW | CS_VREDRAW;
        Class.lpfnWndProc = &WindowProcedure;
        Class.hInstance = Instance;
        Class.hIcon = static_cast<HICON>(LoadImageW(Instance, MAKEINTRESOURCEW(MacawIconResource), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));
        Class.hIconSm = static_cast<HICON>(LoadImageW(Instance, MAKEINTRESOURCEW(MacawIconResource), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
        Class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        Class.lpszClassName = L"MacawWindow";

        if (RegisterClassExW(&Class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            Core::ErrorHandler::ReportWin32(GetLastError(), "Window", "Window class registration failed", Core::ErrorHandler::EErrorLevel::Critical);
        }

        RECT Rectangle{0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)};
        AdjustWindowRectEx(&Rectangle, WS_OVERLAPPEDWINDOW, false, 0);
        mWindow = CreateWindowExW(0, Class.lpszClassName, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Rectangle.right - Rectangle.left, Rectangle.bottom - Rectangle.top, nullptr, nullptr, Instance, this);

        if (mWindow == nullptr) {
            Core::ErrorHandler::ReportWin32(GetLastError(), "Window", "Window creation failed", Core::ErrorHandler::EErrorLevel::Critical);
            Core::ErrorHandler::Fatal("Window", "Window creation failed");
        }

        SendMessageW(mWindow, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(Class.hIcon));
        SendMessageW(mWindow, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(Class.hIconSm));

        RECT Client{};
        GetClientRect(mWindow, &Client);
        mWidth = static_cast<Core::Uint32>(Client.right);
        mHeight = static_cast<Core::Uint32>(Client.bottom);

        mRunning = true;

        if (Visible) {
            ShowWindow(mWindow, SW_SHOWDEFAULT);
            UpdateWindow(mWindow);
        }

        return true;
    }

    /**
     * @brief 대기 중인 Windows 메시지를 처리하고 실행 지속 여부를 반환합니다.
     */
    bool Platform::FWindowsApplication::PumpMessages() {
        MSG Message{};

        while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE)) {
            if (Message.message == WM_QUIT) {
                mRunning = false;
                break;
            }

            TranslateMessage(&Message);
            DispatchMessageW(&Message);
        }

        return mRunning;
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Platform::FWindowsApplication::Shutdown() {
        mMessageHandler = {};

        if (mWindow != nullptr && IsWindow(mWindow)) {
            DestroyWindow(mWindow);
        }

        mWindow = nullptr;
        mRunning = false;
    }

    /**
     * @brief 기본 창 처리 전에 호출할 외부 메시지 핸들러를 교체합니다.
     */
    void Platform::FWindowsApplication::SetMessageHandler(FMessageHandler Handler) {
        mMessageHandler = std::move(Handler);
    }

    /**
     * @brief 네이티브 Win32 창 핸들을 반환합니다.
     */
    HWND Platform::FWindowsApplication::GetWindowHandle() const {
        return mWindow;
    }

    /**
     * @brief 가로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Platform::FWindowsApplication::GetWidth() const {
        return mWidth;
    }

    /**
     * @brief 세로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Platform::FWindowsApplication::GetHeight() const {
        return mHeight;
    }

    /**
     * @brief 창의 클라이언트 영역이 최소화된 크기인지 검사합니다.
     */
    bool Platform::FWindowsApplication::IsMinimized() const {
        return mWidth == 0 || mHeight == 0;
    }

    /**
     * @brief 창 메시지를 인스턴스에 전달하고 크기 및 종료 상태를 갱신합니다.
     */
    LRESULT CALLBACK Platform::FWindowsApplication::WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
        auto* Application{reinterpret_cast<Platform::FWindowsApplication*>(GetWindowLongPtrW(Window, GWLP_USERDATA))};

        if (Message == WM_NCCREATE) {
            const auto* Creation{reinterpret_cast<const CREATESTRUCTW*>(LParam)};
            Application = static_cast<Platform::FWindowsApplication*>(Creation->lpCreateParams);

            SetWindowLongPtrW(Window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Application));
        }

        if (Application != nullptr) {
            if (Application->mMessageHandler && Application->mMessageHandler(Window, Message, WParam, LParam)) {
                return 1;
            }

            if (Message == WM_SIZE) {
                Application->mWidth = LOWORD(LParam);
                Application->mHeight = HIWORD(LParam);

                return 0;
            }

            if (Message == WM_CLOSE) {
                DestroyWindow(Window);

                return 0;
            }

            if (Message == WM_DESTROY) {
                Application->mRunning = false;
                Application->mWindow = nullptr;
                PostQuitMessage(0);

                return 0;
            }
        }

        return DefWindowProcW(Window, Message, WParam, LParam);
    }

}
