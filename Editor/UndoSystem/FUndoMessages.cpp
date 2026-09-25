#include "pch.h"
#include "FUndoMessages.h"

FMessageUndoApply::FMessageUndoApply(const bool InputType) noexcept
    : mBIsUndo(InputType) {
}

FMessageUndoObjectStateChanged::FMessageUndoObjectStateChanged(const FGuid& InputGuid, TArray<Uint8>&& InputData) noexcept
    : mTargetGuid(InputGuid),
      mSavedData(std::move(InputData)) {
}

FMessageUndoObjectSpawned::FMessageUndoObjectSpawned(const FGuid& InputGuid, TArray<Uint8>&& InputData, FString&& InputTargetTypeName) noexcept
    : mTargetGuid(InputGuid),
      mSavedData(std::move(InputData)),
      mTargetTypeName(InputTargetTypeName) {
}

FMessageUndoObjectDestroyed::FMessageUndoObjectDestroyed(const FGuid& InputGuid) noexcept
    : mTargetGuid(InputGuid) {
}
