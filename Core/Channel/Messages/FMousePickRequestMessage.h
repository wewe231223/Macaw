#pragma once

#include <cstdint>

#include "Math/FMath.h"
#include "Core/Base/TypeInfo.h"

struct FMousePickRequestMessage {
    inline static const FTypeInfo TypeInfo{ "FMousePickRequestMessage", nullptr, nullptr};

    static const FTypeInfo& StaticTypeInfo() noexcept;

    std::int32_t mScreenX{0};
    std::int32_t mScreenY{0};
    std::int32_t mViewportLeft{0};
    std::int32_t mViewportTop{0};
    std::uint32_t mViewportWidth{0};
    std::uint32_t mViewportHeight{0};
    FMatrix mViewProjection{};
    FMatrix mView{};

    FMousePickRequestMessage() = default;

    FMousePickRequestMessage(std::int32_t InScreenX, std::int32_t InScreenY, std::int32_t InViewportLeft, std::int32_t InViewportTop, std::uint32_t InViewportWidth, std::uint32_t InViewportHeight, const FMatrix& InViewProjection, const FMatrix& InView) noexcept;
};
