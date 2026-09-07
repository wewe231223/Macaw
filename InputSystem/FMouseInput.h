#pragma once

#include <windows.h>
#include <windowsx.h>

#include "Core/Base/TypeInfo.h"
#include "Core/Channel/FMessageChannel.h"

enum class EMouseButton
{
	Left = 0,
	Right,
	Middle,
    Count
};

enum class EButtonState
{
	None,
	Pressed,
	Held,
	Released
};

struct FMouseInputMessage
{
    EMouseButton Button{ EMouseButton::Left };
    EButtonState State{ EButtonState::None };
    int X{ 0 };
    int Y{ 0 };

    static const FTypeInfo& StaticTypeInfo();
};

class FMouseInput
{
public:
    void Update();

    void ProcessMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN: RawButtonDown[(int)EMouseButton::Left] = true; break;
        case WM_LBUTTONUP:   RawButtonDown[(int)EMouseButton::Left] = false; break;
        case WM_RBUTTONDOWN: RawButtonDown[(int)EMouseButton::Right] = true; break;
        case WM_RBUTTONUP:   RawButtonDown[(int)EMouseButton::Right] = false; break;
        case WM_MBUTTONDOWN: RawButtonDown[(int)EMouseButton::Middle] = true; break;
        case WM_MBUTTONUP:   RawButtonDown[(int)EMouseButton::Middle] = false; break;

        case WM_MOUSEMOVE:
            CurrentMouseX = GET_X_LPARAM(lParam);
            CurrentMouseY = GET_Y_LPARAM(lParam);
            break;

        case WM_MOUSEWHEEL:
            RawWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
            break;
        }
    }

    explicit FMouseInput(FMessageChannel::FSender InInputSender);

    int GetMouseX() const { return CurrentMouseX; }
    int GetMouseY() const { return CurrentMouseY; }
    int GetDeltaX() const { return MouseDeltaX; }
    int GetDeltaY() const { return MouseDeltaY; }

private:

    bool RawButtonDown[(int)EMouseButton::Count]{};
    bool PrevButtonDown[(int)EMouseButton::Count]{};
    EButtonState ButtonStates[(int)EMouseButton::Count]{};

    FMessageChannel::FSender InputSender;

    int CurrentMouseX{ 0 }, CurrentMouseY{ 0 };
    int PrevMouseX{ 0 }, PrevMouseY{ 0 };
    int MouseDeltaX{ 0 }, MouseDeltaY{ 0 };
    int RawWheelDelta{ 0 }, WheelDelta{ 0 };
};