#include "pch.h"
#include "Editor/Input/Messages/FMouseCameraRotateRequestMessage.h"

const FMessageTypeInfo& FMouseCameraRotateRequestMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMouseCameraRotateRequestMessage"};

    return Information;
}

FMouseCameraRotateRequestMessage::FMouseCameraRotateRequestMessage(float InDeltaX, float InDeltaY) noexcept
	: DeltaX(InDeltaX),
	  DeltaY(InDeltaY) {
}
