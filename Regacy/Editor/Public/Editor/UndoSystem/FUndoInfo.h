#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "Core/Base/FGuid.h"
#include "CoreUObject/Reflection/UStruct.h"

struct FObjectStateChangedMessage {
public:
    ReflectTypes(FObjectStateChangedMessage);

public:
    FObjectStateChangedMessage() = default;
    FObjectStateChangedMessage(const FGuid& InGuid, const std::vector<Uint8>& InData);
    FObjectStateChangedMessage(const FGuid& InGuid, std::vector<Uint8>&& InData) noexcept;
    ~FObjectStateChangedMessage() = default;
    FObjectStateChangedMessage(const FObjectStateChangedMessage&) = default;

    FObjectStateChangedMessage& operator=(const FObjectStateChangedMessage&) = default;

    FObjectStateChangedMessage(FObjectStateChangedMessage&&) noexcept = default;

    FObjectStateChangedMessage& operator=(FObjectStateChangedMessage&&) noexcept = default;

public:
    ReflectStruct(FObjectStateChangedMessage);

private:
    ReflectProperty(mTargetGuid, "TargetGuid", EPropertyFlags::None);

    FGuid mTargetGuid{};
    ReflectProperty(mStateData, "StateData", EPropertyFlags::None);

    std::vector<Uint8> mStateData{};

    ReflectMembers();
};
