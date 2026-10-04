#include "pch.h"
#include "Platform/Windows/FWindowsApplication.h"
#include "Platform/Windows/IWindowsMessageHandler.h"
#include <shellapi.h>

namespace {
    constexpr wchar_t WindowTitle[]{L"Macaw Engine"};
    constexpr wchar_t WindowClass[]{L"MacawEngineClass"};
}

FWindowsApplication::~FWindowsApplication() {
    Shutdown();
}

bool FWindowsApplication::Initialize(HINSTANCE Instance, int ShowCommand, IWindowsMessageHandler& Handler, HICON Icon, HICON SmallIcon, HACCEL AcceleratorTable, int Width, int Height) {
    if (mClassRegistered) {
        return false;
    }
    mInstance = Instance;
    mHandler = &Handler;
    mAcceleratorTable = AcceleratorTable;
    mExitRequested = false;
    mExitCode = 0;
    WNDCLASSEXW WindowClassInformation{};
    WindowClassInformation.cbSize = sizeof(WNDCLASSEXW);
    WindowClassInformation.style = CS_HREDRAW | CS_VREDRAW;
    WindowClassInformation.lpfnWndProc = WindowProcedure;
    WindowClassInformation.hInstance = Instance;
    WindowClassInformation.hIcon = Icon;
    WindowClassInformation.hIconSm = SmallIcon;
    WindowClassInformation.hCursor = LoadCursor(nullptr, IDC_ARROW);
    WindowClassInformation.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    WindowClassInformation.lpszClassName = WindowClass;
    if (RegisterClassExW(&WindowClassInformation) == 0) {
        return false;
    }
    mClassRegistered = true;
    const int PositionX{(GetSystemMetrics(SM_CXSCREEN) - Width) / 2};
    const int PositionY{(GetSystemMetrics(SM_CYSCREEN) - Height) / 2};
    mWindowHandle = CreateWindowExW(WS_EX_APPWINDOW, WindowClass, WindowTitle, WS_POPUP, PositionX, PositionY, Width, Height, nullptr, nullptr, Instance, this);
    if (mWindowHandle == nullptr) {
        Shutdown();
        return false;
    }
    ShowWindow(mWindowHandle, ShowCommand);
    UpdateWindow(mWindowHandle);
    DragAcceptFiles(mWindowHandle, TRUE);
    return true;
}

void FWindowsApplication::Shutdown() {
    mHandler = nullptr;
    mExitRequested = true;
    if (mWindowHandle != nullptr) {
        DestroyWindow(mWindowHandle);
        mWindowHandle = nullptr;
    }
    if (mClassRegistered) {
        UnregisterClassW(WindowClass, mInstance);
        mClassRegistered = false;
    }
    mAcceleratorTable = nullptr;
    mInstance = nullptr;
}

bool FWindowsApplication::PumpMessages() {
    MSG Message{};
    while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE)) {
        if (Message.message == WM_QUIT) {
            mExitRequested = true;
            mExitCode = static_cast<int>(Message.wParam);
            return false;
        }
        if (!TranslateAcceleratorW(Message.hwnd, mAcceleratorTable, &Message)) {
            TranslateMessage(&Message);
            DispatchMessageW(&Message);
        }
    }
    return !mExitRequested;
}

int FWindowsApplication::GetExitCode() const {
    return mExitCode;
}

HWND FWindowsApplication::GetWindowHandle() const {
    return mWindowHandle;
}

LRESULT CALLBACK FWindowsApplication::WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
    FWindowsApplication* Application{reinterpret_cast<FWindowsApplication*>(GetWindowLongPtrW(WindowHandle, GWLP_USERDATA))};
    if (Message == WM_NCCREATE) {
        const CREATESTRUCTW* Creation{reinterpret_cast<const CREATESTRUCTW*>(LParam)};
        Application = static_cast<FWindowsApplication*>(Creation->lpCreateParams);
        Application->mWindowHandle = WindowHandle;
        SetWindowLongPtrW(WindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(Application));
    }
    if (Application == nullptr) {
        return DefWindowProcW(WindowHandle, Message, WParam, LParam);
    }
    const LRESULT Result{Application->mHandler != nullptr ? Application->mHandler->ProcessWindowMessage(WindowHandle, Message, WParam, LParam) : DefWindowProcW(WindowHandle, Message, WParam, LParam)};
    if (Message == WM_DESTROY && Application->mHandler != nullptr) {
        PostQuitMessage(0);
    } else if (Message == WM_NCDESTROY) {
        SetWindowLongPtrW(WindowHandle, GWLP_USERDATA, 0);
        Application->mWindowHandle = nullptr;
    }
    return Result;
}
