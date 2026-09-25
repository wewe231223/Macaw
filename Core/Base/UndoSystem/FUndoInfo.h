#pragma once
#include "../FGuid.h"
#include "../../Base/TypeInfo.h"

struct FObjectStateChangedMessage {
public:
    FGuid TargetGuid{};
    std::vector<Uint8> StateData{};

    inline static const FTypeInfo TypeInfo{ "FObjectStateChangedMessage", nullptr, nullptr};

    static const FTypeInfo& StaticTypeInfo() noexcept;

    FObjectStateChangedMessage(const FGuid& InGuid, const std::vector<Uint8>& InData);

    FObjectStateChangedMessage(const FGuid& InGuid, std::vector<Uint8>&& InData) noexcept;

    FObjectStateChangedMessage() = default;
    ~FObjectStateChangedMessage() = default;
    FObjectStateChangedMessage(const FObjectStateChangedMessage&) = default;
    FObjectStateChangedMessage& operator=(const FObjectStateChangedMessage&) = default;
    FObjectStateChangedMessage(FObjectStateChangedMessage&&) noexcept = default;
    FObjectStateChangedMessage& operator=(FObjectStateChangedMessage&&) noexcept = default;
};
