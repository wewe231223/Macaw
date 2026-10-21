#include "pch.h"
#include "Editor/Input/Messages/FMouseCameraMoveRequestMessage.h"

const FMessageTypeInfo& FMouseCameraMoveRequestMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMouseCameraMoveRequestMessage"};

    return Information;
}

FMouseCameraMoveRequestMessage::FMouseCameraMoveRequestMessage(float InDeltaX, float InDeltaY) noexcept
	: DeltaX(InDeltaX),
	  DeltaY(InDeltaY) {
}
