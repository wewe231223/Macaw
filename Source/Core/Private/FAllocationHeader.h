#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

struct FAllocationHeader {
public:
    ReflectTypes(FAllocationHeader);

public:
    ReflectStruct(FAllocationHeader);

private:
    ReflectMembers();

public:
    void* mRaw{};
    ReflectProperty(mRaw, "Raw", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
