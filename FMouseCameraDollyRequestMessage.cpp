#include "pch.h"
#include "FMouseCameraDollyRequestMessage.h"

const FTypeInfo& FMouseCameraDollyRequestMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FMouseCameraDollyRequestMessage::FMouseCameraDollyRequestMessage(float InSteps) noexcept
    : Steps(InSteps) {
}
