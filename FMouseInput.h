#pragma once

#include <cstdint>
#include <optional>

#include <Windows.h>

#include "Core/Channel/FMessageChannel.h"

enum EMouseSide : uint32 {
    Left, Right, MAX
};

class FMouseInput
{
public:
	void InitializeWorldCommandSender(FMessageChannel::FSender&& InSender);

	void ProcessWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);

	void DispatchPendingWorldCommands(std::uint32_t ViewportWidth, std::uint32_t ViewportHeight, bool bMouseCaptureByUI);

private:
    std::optional<FMessageChannel::FSender> WorldCommandSender;

	bool bLeftButtonDown = false;
    bool bLeftClickPending = false;
    bool bLeftClickReleasedPending = false; 

    std::int32_t LeftClickX = 0;
    std::int32_t LeftClickY = 0;

    bool bRightButtonDown = false;
    bool bRightButtonPressedPending = false;
    bool bRightButtonReleasedPending = false;
    bool bRightDragAuthorized = false;

    bool bHasLastMousePosition = false;
    std::int32_t LastMouseX = 0;
    std::int32_t LastMouseY = 0;

    float PendingRotateDeltaX = 0.0f;
    float PendingRotateDeltaY = 0.0f;

    struct DragCapture {
        POINT start;
        POINT current; 
    };

	TFixedArray<DragCapture, static_cast<size_t>(EMouseSide::MAX)> ClickCaptures{};


	DragCapture[Left].start = ....;
    ClickCaptures[Left].current = .....;


};
