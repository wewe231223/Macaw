#pragma once

#include "Core/Types.h"

#include <Windows.h>
#include <functional>

class FWindowsApplication {
public:
    using FMessageHandler = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

public:
    FWindowsApplication();
    ~FWindowsApplication();
    FWindowsApplication(const FWindowsApplication&) = delete;
    FWindowsApplication& operator=(const FWindowsApplication&) = delete;

public:
    bool Create(const wchar_t* Title, Uint32 Width, Uint32 Height, bool Visible = true);
    bool PumpMessages();
    void Shutdown();

    void SetMessageHandler(FMessageHandler Handler);

    HWND GetWindowHandle() const;
    Uint32 GetWidth() const;
    Uint32 GetHeight() const;
    bool IsMinimized() const;

private:
    static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

private:
    HWND mWindow{};
    Uint32 mWidth{};
    Uint32 mHeight{};

    bool mRunning{};

    FMessageHandler mMessageHandler{};
};
