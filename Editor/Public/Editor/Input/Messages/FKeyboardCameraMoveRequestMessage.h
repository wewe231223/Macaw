#pragma once
#include "Core/Channel/FMessageTypeInfo.h"

struct FKeyboardCameraMoveRequestMessage {

    static const FMessageTypeInfo& StaticTypeInfo() noexcept;

    float ForwardAxis{0.0f};
    float RightAxis{0.0f};
    float UpAxis{0.0f};
    float DeltaTime{0.0f};

    FKeyboardCameraMoveRequestMessage() = default;

    FKeyboardCameraMoveRequestMessage(float InForwardAxis, float InRightAxis, float InDeltaTime) noexcept;
};
