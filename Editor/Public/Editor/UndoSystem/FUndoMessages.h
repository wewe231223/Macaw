#pragma once
#include "Core/Base/FName.h"
#include "Core/Base/FGuid.h"
#include "Core/Channel/FMessageTypeInfo.h"
#include "Core/Common.h"

struct FMessageUndoApply {
public:
    FMessageUndoApply(const bool InputType) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageUndoApply);

public:
    bool mBIsUndo{}; // true = undo, false = redo
};

struct FMessageUndoObjectStateChanged {
public:
    FMessageUndoObjectStateChanged(const FGuid& InputGuid, TArray<Uint8>&& InputData) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectStateChanged);

public:
    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};
};

struct FMessageUndoObjectSpawned {
public:
    FMessageUndoObjectSpawned(const FGuid& InputGuid, TArray<Uint8>&& InputData, FName InputTargetTypeName) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectSpawned);

public:
    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};
    FName mTargetTypeName{};
};

struct FMessageUndoObjectDestroyed {
public:
    FMessageUndoObjectDestroyed(const FGuid& InputGuid) noexcept;
    DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectDestroyed);

public:
    FGuid mTargetGuid{};
};
