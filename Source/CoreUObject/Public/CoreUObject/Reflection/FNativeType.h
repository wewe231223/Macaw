#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>
#include <string_view>

struct FNativeType {
public:
    ReflectTypes(FNativeType);

public:
    ReflectStruct(FNativeType);

private:
    ReflectMembers();

public:
    std::size_t mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mAlignment{};
    ReflectProperty(mAlignment, "Alignment", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    std::string_view mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
