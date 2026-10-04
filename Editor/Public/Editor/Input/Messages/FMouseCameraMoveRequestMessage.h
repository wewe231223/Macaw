#pragma once
#include "Core/Channel/FMessageTypeInfo.h"

struct FMouseCameraMoveRequestMessage {

    static const FMessageTypeInfo& StaticTypeInfo() noexcept;

    float DeltaX{0.0f};
    float DeltaY{0.0f};

    FMouseCameraMoveRequestMessage() = default;

    FMouseCameraMoveRequestMessage(float InDeltaX, float InDeltaY) noexcept;
};
