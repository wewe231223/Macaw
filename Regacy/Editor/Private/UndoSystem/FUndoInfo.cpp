#include "pch.h"
#include "Editor/UndoSystem/FUndoInfo.h"

FObjectStateChangedMessage::FObjectStateChangedMessage(const FGuid& InGuid, const std::vector<Uint8>& InData)
	: mTargetGuid(InGuid),
	  mStateData(InData) {
}

FObjectStateChangedMessage::FObjectStateChangedMessage(const FGuid& InGuid, std::vector<Uint8>&& InData) noexcept
	: mTargetGuid(InGuid),
	  mStateData(std::move(InData)) {
}
