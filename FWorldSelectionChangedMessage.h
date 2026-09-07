#pragma once

#include "Core/Base/FObjectHandle.h"
#include "Core/Base/TypeInfo.h"

struct FWorldSelectionChangedMessage
{
    inline static const FTypeInfo TypeInfo{
        "FWorldSelectionChangedMessage",
        nullptr,
        nullptr
    };

    static const FTypeInfo& StaticTypeInfo() noexcept
    {
        return TypeInfo;
    }

    FObjectHandle SelectedComponentHandle{};

    FWorldSelectionChangedMessage() = default;

    explicit FWorldSelectionChangedMessage(
        FObjectHandle InSelectedComponentHandle) noexcept
        : SelectedComponentHandle(InSelectedComponentHandle)
    {
    }
};