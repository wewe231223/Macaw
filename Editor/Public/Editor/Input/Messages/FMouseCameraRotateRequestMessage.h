#pragma once
#include "Core/Channel/FMessageTypeInfo.h"

struct FMouseCameraRotateRequestMessage {
    static const FMessageTypeInfo& StaticTypeInfo() noexcept;

    float DeltaX{0.0f};
    float DeltaY{0.0f};

    FMouseCameraRotateRequestMessage() = default;

    FMouseCameraRotateRequestMessage(float InDeltaX, float InDeltaY) noexcept;
};
