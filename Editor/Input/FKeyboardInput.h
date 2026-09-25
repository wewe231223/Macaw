#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <Windows.h>

#include "Core/Channel/FMessageChannel.h"
#include "EKeyState.h"

struct FViewportKeyboardNavigationInput {
    float mForwardAxis{0.0f};
    float mRightAxis{0.0f};
    float mUpAxis{0.0f};
    float mDeltaTime{0.0f};
};

class FKeyboardInput {
public:
    FKeyboardInput();

    void InitializeWorldCommandSender(FMessageChannel::FSender&& Sender);

    void ProcessWindowMessage(UINT Message, WPARAM WParam, LPARAM LParam);

    FViewportKeyboardNavigationInput ConsumeViewportNavigation(float DeltaTime, bool BKeyboardCaptureByUi);

    EKeyState GetKeyState(std::uint8_t VirtualKey) const;

private:
    static constexpr std::size_t KeyCount{256};

    bool IsHeld(std::uint8_t VirtualKey) const;
    void AdvanceKeyStates();
    void ResetKeyStates();

    std::array<EKeyState, KeyCount> mKeyStates{};
    std::optional<FMessageChannel::FSender> mWorldCommandSender{};
};
