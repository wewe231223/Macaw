#include "pch.h"
#include "Editor/UndoSystem/FUndoMessages.h"

const FMessageTypeInfo& FMessageUndoObjectStateChanged::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageUndoObjectStateChanged"};

    return Information;
}

const FMessageTypeInfo& FMessageUndoObjectSpawned::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageUndoObjectSpawned"};

    return Information;
}

const FMessageTypeInfo& FMessageUndoObjectDestroyed::StaticTypeInfo() noexcept {
    static const FMessageTypeInfo Information{"FMessageUndoObjectDestroyed"};

    return Information;
}

FMessageUndoObjectStateChanged::FMessageUndoObjectStateChanged(const FGuid& InputGuid, TArray<Uint8>&& InputData) noexcept
	: mTargetGuid(InputGuid),
	  mSavedData(std::move(InputData)) {
}

FMessageUndoObjectSpawned::FMessageUndoObjectSpawned(const FGuid& InputGuid, TArray<Uint8>&& InputData, FName InputTargetTypeName) noexcept
	: mTargetGuid(InputGuid),
	  mSavedData(std::move(InputData)),
	  mTargetTypeName(InputTargetTypeName) {
}

FMessageUndoObjectDestroyed::FMessageUndoObjectDestroyed(const FGuid& InputGuid) noexcept
	: mTargetGuid(InputGuid) {
}
