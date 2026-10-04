#pragma once
#include "Core/Channel/FMessageTypeInfo.h"

struct FMouseCameraDollyRequestMessage {

    static const FMessageTypeInfo& StaticTypeInfo() noexcept;

    float Steps{0.0f};

    FMouseCameraDollyRequestMessage() = default;

    explicit FMouseCameraDollyRequestMessage(float InSteps) noexcept;
};
