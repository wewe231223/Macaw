#include "pch.h"
#include "FUndoInfo.h"

const FTypeInfo& FObjectStateChangedMessage::StaticTypeInfo() noexcept {
    return TypeInfo;
}

FObjectStateChangedMessage::FObjectStateChangedMessage(const FGuid& InGuid, const std::vector<Uint8>& InData)
    : TargetGuid(InGuid),
      StateData(InData) {
}

FObjectStateChangedMessage::FObjectStateChangedMessage(const FGuid& InGuid, std::vector<Uint8>&& InData) noexcept
    : TargetGuid(InGuid),
      StateData(std::move(InData)) {
}
