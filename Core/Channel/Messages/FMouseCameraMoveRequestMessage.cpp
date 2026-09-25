#include "pch.h"
#include "FMouseCameraMoveRequestMessage.h"

const FTypeInfo& FMouseCameraMoveRequestMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FMouseCameraMoveRequestMessage::FMouseCameraMoveRequestMessage(float InDeltaX, float InDeltaY) noexcept
    : DeltaX(InDeltaX),
      DeltaY(InDeltaY) {
}
