#pragma once

#include <Windows.h>
#include "Platform/IPlatformApplication.h"

class IWindowsMessageHandler;

class FWindowsApplication final : public IPlatformApplication {
public:
    FWindowsApplication() = default;
    ~FWindowsApplication() override;
    FWindowsApplication(const FWindowsApplication&) = delete;
    FWindowsApplication& operator=(const FWindowsApplication&) = delete;
    FWindowsApplication(FWindowsApplication&&) = delete;
    FWindowsApplication& operator=(FWindowsApplication&&) = delete;

public:
    bool Initialize(HINSTANCE Instance, int ShowCommand, IWindowsMessageHandler& Handler, HICON Icon, HICON SmallIcon, HACCEL AcceleratorTable, int Width, int Height);
    void Shutdown();
    bool PumpMessages() override;
    int GetExitCode() const override;
    HWND GetWindowHandle() const;

private:
    static LRESULT CALLBACK WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

private:
    HINSTANCE mInstance{};
    HWND mWindowHandle{};
    HACCEL mAcceleratorTable{};
    IWindowsMessageHandler* mHandler{};
    int mExitCode{};
    bool mClassRegistered{};
    bool mExitRequested{};
};
