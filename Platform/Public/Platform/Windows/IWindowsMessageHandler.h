#pragma once

#include <Windows.h>

class IWindowsMessageHandler {
public:
    virtual ~IWindowsMessageHandler() = default;

public:
    virtual LRESULT ProcessWindowMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) = 0;
};
