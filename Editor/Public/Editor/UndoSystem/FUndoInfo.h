#pragma once

#include "Core/Base/FGuid.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Editor/UndoSystem/FUndoInfo.generated.h"

USTRUCT()
struct FObjectStateChangedMessage {
public:
    GENERATED_TYPES(FObjectStateChangedMessage)

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
    GENERATED_BODY(FObjectStateChangedMessage)

private:
    UPROPERTY(Name = "TargetGuid")
    FGuid mTargetGuid{};
    UPROPERTY(Name = "StateData")
    std::vector<Uint8> mStateData{};
};
