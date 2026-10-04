#pragma once

#include <string_view>

struct FMessageTypeInfo {
    std::string_view mTypeName{};

    bool IsExactlyA(const FMessageTypeInfo* Type) const noexcept;
};

#define JG_DECLARE_CHANNEL_MESSAGE(MessageType) \
    static const FMessageTypeInfo& StaticTypeInfo() noexcept; \
    MessageType() = default; \
    ~MessageType() = default; \
    MessageType(const MessageType&) = default; \
    MessageType& operator=(const MessageType&) = default; \
    MessageType(MessageType&&) noexcept = default; \
    MessageType& operator=(MessageType&&) noexcept = default
