#pragma once
#include "../FGuid.h" 
#include "../../Base/TypeInfo.h"
#include "../../../Common.h"

struct FMessageUndoObjectStateChanged
{
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectStateChanged);

    FGuid TargetGuid;
    TArray<uint8> StateData;

    FMessageUndoObjectStateChanged(const FGuid& InputGuid, const TArray<uint8>& InputData)
        : TargetGuid(InputGuid), StateData(InputData) {}

    FMessageUndoObjectStateChanged(const FGuid& InputGuid, TArray<uint8>&& InputData) noexcept
        : TargetGuid(InputGuid), StateData(std::move(InputData)) {}
};


struct FMessageUndoObjectSpawned
{
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectSpawned);

    FGuid TargetGuid;
    TArray<uint8> SavedData;

    FMessageUndoObjectSpawned(const FGuid& InputGuid, const TArray<uint8>& InputData)
        : TargetGuid(InputGuid), SavedData(InputData) {}

    FMessageUndoObjectSpawned(const FGuid& InputGuid, TArray<uint8>&& InputData) noexcept
        : TargetGuid(InputGuid), SavedData(std::move(InputData)) {}
};


struct FMessageUndoObjectDestroyed
{
    JG_DECLARE_CHANNEL_MESSAGE(FMessageUndoObjectDestroyed);

    FGuid TargetGuid;

    FMessageUndoObjectDestroyed(const FGuid& InputGuid)
        : TargetGuid(InputGuid) {}
};