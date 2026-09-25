#include "pch.h"
#include "FMousePickRequestMessage.h"

const FTypeInfo& FMousePickRequestMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FMousePickRequestMessage::FMousePickRequestMessage(std::int32_t InScreenX, std::int32_t InScreenY, std::int32_t InViewportLeft, std::int32_t InViewportTop, std::uint32_t InViewportWidth, std::uint32_t InViewportHeight, const FMatrix& InViewProjection, const FMatrix& InView) noexcept
    : mScreenX(InScreenX),
      mScreenY(InScreenY),
      mViewportLeft(InViewportLeft),
      mViewportTop(InViewportTop),
      mViewportWidth(InViewportWidth),
      mViewportHeight(InViewportHeight),
      mViewProjection(InViewProjection),
      mView(InView) {
}
