#pragma once

#include "Core/Base/TypeInfo.h"

struct FMouseCameraMoveRequestMessage {
    inline static const FTypeInfo TypeInfo{ "FMouseCameraMoveRequestMessage", nullptr, nullptr};

    static const FTypeInfo& StaticTypeInfo() noexcept;

    float DeltaX{0.0f};
    float DeltaY{0.0f};

    FMouseCameraMoveRequestMessage() = default;

    FMouseCameraMoveRequestMessage(float InDeltaX, float InDeltaY) noexcept;
};
