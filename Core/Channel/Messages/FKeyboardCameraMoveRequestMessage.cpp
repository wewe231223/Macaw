#include "pch.h"
#include "FKeyboardCameraMoveRequestMessage.h"

const FTypeInfo& FKeyboardCameraMoveRequestMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FKeyboardCameraMoveRequestMessage::FKeyboardCameraMoveRequestMessage(float InForwardAxis, float InRightAxis, float InDeltaTime) noexcept
    : ForwardAxis(InForwardAxis),
      RightAxis(InRightAxis),
      DeltaTime(InDeltaTime) {
}
