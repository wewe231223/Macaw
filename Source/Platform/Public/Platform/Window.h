#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <Windows.h>
#include <functional>

class FWindowsApplication {
public:
    ReflectTypes(FWindowsApplication);

public:
    using FMessageHandler = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

public:
    FWindowsApplication();
    ~FWindowsApplication();
    FWindowsApplication(const FWindowsApplication&) = delete;
    FWindowsApplication& operator=(const FWindowsApplication&) = delete;

public:
    ReflectStruct(FWindowsApplication);

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
    ReflectMembers();

private:
    HWND mWindow{};
    ReflectProperty(mWindow, "Window", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mWidth{};
    ReflectProperty(mWidth, "Width", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mHeight{};
    ReflectProperty(mHeight, "Height", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    bool mRunning{};
    ReflectProperty(mRunning, "Running", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FMessageHandler mMessageHandler{};
    ReflectProperty(mMessageHandler, "MessageHandler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
