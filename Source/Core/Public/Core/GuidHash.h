#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"

struct FGuidHash {
public:
    ReflectTypes(FGuidHash);

public:
    ReflectStruct(FGuidHash);

    std::size_t operator()(const FGuid& Value) const noexcept;

private:
    ReflectMembers();
};
