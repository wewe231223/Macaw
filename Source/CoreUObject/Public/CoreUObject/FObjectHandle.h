#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <limits>

struct FObjectHandle {
public:
    ReflectTypes(FObjectHandle);

public:
    ReflectStruct(FObjectHandle);

    bool IsValid() const;
    bool operator==(const FObjectHandle& Other) const;

private:
    ReflectMembers();

public:
    Uint32 mIndex{std::numeric_limits<Uint32>::max()};
    ReflectProperty(mIndex, "Index", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Uint32 mGeneration{};
    ReflectProperty(mGeneration, "Generation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
