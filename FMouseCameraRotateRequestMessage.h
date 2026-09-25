#pragma once

#include "Core/Base/TypeInfo.h"

struct FMouseCameraRotateRequestMessage {
    inline static const FTypeInfo TypeInfo{ "FMouseCameraRotateRequestMessage", nullptr, nullptr};

    static const FTypeInfo& StaticTypeInfo() noexcept;

    float DeltaX{0.0f};
    float DeltaY{0.0f};

    FMouseCameraRotateRequestMessage() = default;

    FMouseCameraRotateRequestMessage(float InDeltaX, float InDeltaY) noexcept;
};
