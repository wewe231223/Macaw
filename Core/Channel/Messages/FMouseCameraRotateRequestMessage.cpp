#include "pch.h"
#include "FMouseCameraRotateRequestMessage.h"

const FTypeInfo& FMouseCameraRotateRequestMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FMouseCameraRotateRequestMessage::FMouseCameraRotateRequestMessage(float InDeltaX, float InDeltaY) noexcept
    : DeltaX(InDeltaX),
      DeltaY(InDeltaY) {
}
