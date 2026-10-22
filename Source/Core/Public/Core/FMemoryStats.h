#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

struct FMemoryStats {
public:
    ReflectTypes(FMemoryStats);

public:
    ReflectStruct(FMemoryStats);

private:
    ReflectMembers();

public:
    std::size_t mLiveObjects{};
    ReflectProperty(mLiveObjects, "LiveObjects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mCreatedObjects{};
    ReflectProperty(mCreatedObjects, "CreatedObjects", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    std::size_t mHeapBytes{};
    ReflectProperty(mHeapBytes, "HeapBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mPeakHeapBytes{};
    ReflectProperty(mPeakHeapBytes, "PeakHeapBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
