#include "pch.h"
#include "Editor/Input/Messages/FMouseCameraDollyRequestMessage.h"

const FMessageTypeInfo& FMouseCameraDollyRequestMessage::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMouseCameraDollyRequestMessage"};

    return Information;
}

FMouseCameraDollyRequestMessage::FMouseCameraDollyRequestMessage(float InSteps) noexcept
	: Steps(InSteps) {
}
