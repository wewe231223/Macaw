#include "PCH.h"

#include "FMouseInput.h"
#include "FMousePickRequestMessage.h"
#include "FMouseCameraRotateRequestMessage.h"

namespace
{
    std::int32_t GetMouseX(LPARAM LParam)
    {
        return static_cast<std::int32_t>(
            static_cast<std::int16_t>(LOWORD(LParam)));
    }

    std::int32_t GetMouseY(LPARAM LParam)
    {
        return static_cast<std::int32_t>(
            static_cast<std::int16_t>(HIWORD(LParam)));
    }
}

void FMouseInput::InitializeWorldCommandSender(
    FMessageChannel::FSender&& InSender)
{
    WorldCommandSender.emplace(std::move(InSender));
}

void FMouseInput::ProcessWindowMessage(
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    UNREFERENCED_PARAMETER(WParam);

    switch (Message)
    {
    case WM_LBUTTONDOWN:
    {
        bLeftClickPending = true;
        LeftClickX = GetMouseX(LParam);
        LeftClickY = GetMouseY(LParam);
        break;
    }

    case WM_RBUTTONDOWN:
    {
        bRightButtonDown = true;
        bRightButtonPressedPending = true;
        bRightButtonReleasedPending = false;
        bRightDragAuthorized = false;

        PendingRotateDeltaX = 0.0f;
        PendingRotateDeltaY = 0.0f;

        LastMouseX = GetMouseX(LParam);
        LastMouseY = GetMouseY(LParam);
        bHasLastMousePosition = true;
        break;
    }

    case WM_MOUSEMOVE:
    {
        const std::int32_t CurrentMouseX = GetMouseX(LParam);
        const std::int32_t CurrentMouseY = GetMouseY(LParam);

        if (bRightButtonDown && bHasLastMousePosition)
        {
            PendingRotateDeltaX +=
                static_cast<float>(CurrentMouseX - LastMouseX);

            PendingRotateDeltaY -=
                static_cast<float>(CurrentMouseY - LastMouseY);
        }

        LastMouseX = CurrentMouseX;
        LastMouseY = CurrentMouseY;
        bHasLastMousePosition = true;
        break;
    }

    case WM_RBUTTONUP:
    {
        bRightButtonDown = false;
        bRightButtonReleasedPending = true;
        bHasLastMousePosition = false;
        break;
    }

    case WM_KILLFOCUS:
    {
        bLeftClickPending = false;
        bRightButtonDown = false;
        bRightButtonPressedPending = false;
        bRightButtonReleasedPending = false;
        bRightDragAuthorized = false;
        bHasLastMousePosition = false;
        PendingRotateDeltaX = 0.0f;
        PendingRotateDeltaY = 0.0f;
        break;
    }

    default:
        break;
    }
}

void FMouseInput::DispatchPendingWorldCommands(
    std::uint32_t ViewportWidth,
    std::uint32_t ViewportHeight,
    bool bMouseCapturedByUI)
{
    if (!WorldCommandSender.has_value())
    {
        return;
    }

    if (bLeftClickPending)
    {
        if (!bMouseCapturedByUI)
        {
            WorldCommandSender->TryEmplace<FMousePickRequestMessage>(
                LeftClickX,
                LeftClickY,
                ViewportWidth,
                ViewportHeight);
        }

        bLeftClickPending = false;
    }

    if (bRightButtonPressedPending)
    {
        bRightDragAuthorized = !bMouseCapturedByUI;
        bRightButtonPressedPending = false;

        if (!bRightDragAuthorized)
        {
            PendingRotateDeltaX = 0.0f;
            PendingRotateDeltaY = 0.0f;
        }
    }

    if (bRightDragAuthorized &&
        (PendingRotateDeltaX != 0.0f ||
            PendingRotateDeltaY != 0.0f))
    {
        WorldCommandSender->TryEmplace<
            FMouseCameraRotateRequestMessage>(
                PendingRotateDeltaX,
                PendingRotateDeltaY);

        PendingRotateDeltaX = 0.0f;
        PendingRotateDeltaY = 0.0f;
    }

    if (bRightButtonReleasedPending)
    {
        bRightDragAuthorized = false;
        bRightButtonReleasedPending = false;
        PendingRotateDeltaX = 0.0f;
        PendingRotateDeltaY = 0.0f;
    }
}