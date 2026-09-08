#pragma once
#include "../FGuid.h" 
#include "../../Base/TypeInfo.h"

struct FMessageObjectStateChanged
{
    JG_DECLARE_CHANNEL_MESSAGE(FMessageObjectStateChanged);

    FGuid TargetGuid;
    TArray<uint8> StateData;

    FMessageObjectStateChanged(const FGuid& InGuid, const TArray<uint8>& InData)
        : TargetGuid(InGuid), StateData(InData)
    {}

    FMessageObjectStateChanged(const FGuid& InGuid, TArray<uint8>&& InData) noexcept
        : TargetGuid(InGuid), StateData(std::move(InData))
    {}
};