#pragma once
#include "../FGuid.h"
#include "../../Base/TypeInfo.h"
#include "../../../Common.h"

struct FMessageUndoApply {
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoApply);

    bool mBIsUndo{}; // true = undo, false = redo

    FMessageUndoApply(const bool InputType) noexcept;
};

struct FMessageUndoObjectStateChanged {
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectStateChanged);

    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};

    FMessageUndoObjectStateChanged(const FGuid& InputGuid, TArray<Uint8>&& InputData) noexcept;
};

struct FMessageUndoObjectSpawned {
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectSpawned);

    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};
    FString mTargetTypeName{};

    FMessageUndoObjectSpawned(const FGuid& InputGuid, TArray<Uint8>&& InputData, FString&& InputTargetTypeName) noexcept;
};

struct FMessageUndoObjectDestroyed {
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectDestroyed);

    FGuid mTargetGuid{};

    FMessageUndoObjectDestroyed(const FGuid& InputGuid) noexcept;
};
