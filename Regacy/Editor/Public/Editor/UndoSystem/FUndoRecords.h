#pragma once
#include "Core/Base/FName.h"
#include "Editor/UndoSystem/IUndoRecord.h"
#include "Core/Base/FGuid.h"

class FRecordObjectState : public IUndoRecord {
public:
    FRecordObjectState(const FGuid& InGuid, const TArray<Uint8>& InBefore, const TArray<Uint8>& InAfter);

public:
    virtual void ApplyUndo(IUndoContext* Context) override;
    virtual void ApplyRedo(IUndoContext* Context) override;

private:
    FGuid mTargetGuid{};

    TArray<Uint8> mBeforeData{};
    TArray<Uint8> mAfterData{};
};

class FRecordObjectSpawned : public IUndoRecord {
public:
    FRecordObjectSpawned(FGuid InputGuid, const TArray<Uint8>& InputSavedData, FName InputTargetTypeName);

public:
    virtual void ApplyUndo(IUndoContext* Context) override;
    virtual void ApplyRedo(IUndoContext* Context) override;

private:
    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};
    FName mTargetTypeName{};
};

class FRecordObjectDestroyed : public IUndoRecord {
public:
    FRecordObjectDestroyed(FGuid InputGuid, TArray<Uint8> InputSavedData, FName InputTargetTypeName);

public:
    virtual void ApplyUndo(IUndoContext* Context) override;
    virtual void ApplyRedo(IUndoContext* Context) override;

private:
    FGuid mTargetGuid{};
    TArray<Uint8> mSavedData{};
    FName mTargetTypeName{};
};
