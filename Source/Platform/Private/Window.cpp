#include "Platform/Window.h"
#include "Core/Base/ErrorHandler.h"

#include <utility>

FWindowsApplication::FWindowsApplication() = default;

FWindowsApplication::~FWindowsApplication() {
    Shutdown();
}

bool FWindowsApplication::Create(const wchar_t* Title, Uint32 Width, Uint32 Height, bool Visible) {
    if (mWindow != nullptr) {
        ErrorHandler::Fatal("Window", "The application window already exists");
    }

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const HINSTANCE Instance{GetModuleHandleW(nullptr)};

    WNDCLASSEXW Class{};
    Class.cbSize = sizeof(Class);
    Class.style = CS_HREDRAW | CS_VREDRAW;
    Class.lpfnWndProc = &WindowProcedure;
    Class.hInstance = Instance;
    Class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    Class.lpszClassName = L"MacawWeek1Window";

    if (RegisterClassExW(&Class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        ErrorHandler::ReportWin32(GetLastError(), "Window", "Window class registration failed", ErrorHandler::EErrorLevel::Critical);
    }

    RECT Rectangle{0, 0, static_cast<LONG>(Width), static_cast<LONG>(Height)};
    AdjustWindowRectEx(&Rectangle, WS_OVERLAPPEDWINDOW, false, 0);
    mWindow = CreateWindowExW(0, Class.lpszClassName, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, Rectangle.right - Rectangle.left, Rectangle.bottom - Rectangle.top, nullptr, nullptr, Instance, this);

    if (mWindow == nullptr) {
        ErrorHandler::ReportWin32(GetLastError(), "Window", "Window creation failed", ErrorHandler::EErrorLevel::Critical);
        ErrorHandler::Fatal("Window", "Window creation failed");
    }

    RECT Client{};
    GetClientRect(mWindow, &Client);
    mWidth = static_cast<Uint32>(Client.right);
    mHeight = static_cast<Uint32>(Client.bottom);

    mRunning = true;

    if (Visible) {
        ShowWindow(mWindow, SW_SHOWDEFAULT);
        UpdateWindow(mWindow);
    }

    return true;
}

bool FWindowsApplication::PumpMessages() {
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

void FWindowsApplication::Shutdown() {
    mMessageHandler = {};

    if (mWindow != nullptr && IsWindow(mWindow)) {
        DestroyWindow(mWindow);
    }

    mWindow = nullptr;
    mRunning = false;
}

void FWindowsApplication::SetMessageHandler(FMessageHandler Handler) {
    mMessageHandler = std::move(Handler);
}

HWND FWindowsApplication::GetWindowHandle() const {
    return mWindow;
}

Uint32 FWindowsApplication::GetWidth() const {
    return mWidth;
}

Uint32 FWindowsApplication::GetHeight() const {
    return mHeight;
}

bool FWindowsApplication::IsMinimized() const {
    return mWidth == 0 || mHeight == 0;
}

LRESULT CALLBACK FWindowsApplication::WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    auto* Application{reinterpret_cast<FWindowsApplication*>(GetWindowLongPtrW(Window, GWLP_USERDATA))};

    if (Message == WM_NCCREATE) {
        const auto* Creation{reinterpret_cast<const CREATESTRUCTW*>(LParam)};
        Application = static_cast<FWindowsApplication*>(Creation->lpCreateParams);

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
