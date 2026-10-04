#include "pch.h"
#include "Editor/Input/Messages/FKeyboardCameraMoveRequestMessage.h"

const FMessageTypeInfo& FKeyboardCameraMoveRequestMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FKeyboardCameraMoveRequestMessage"};
    return Information;
}

FKeyboardCameraMoveRequestMessage::FKeyboardCameraMoveRequestMessage(float InForwardAxis, float InRightAxis, float InDeltaTime) noexcept
    : ForwardAxis(InForwardAxis),
      RightAxis(InRightAxis),
      DeltaTime(InDeltaTime) {
}
