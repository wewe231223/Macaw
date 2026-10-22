#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/FMemoryStats.h"

class FMemory {
public:
    ReflectTypes(FMemory);

public:
    ReflectStruct(FMemory);

    static void* Allocate(std::size_t Size, std::size_t Alignment);
    static void Free(void* Pointer) noexcept;

    static void ObjectCreated();
    static void ObjectDestroyed();

    static FMemoryStats GetStats();

private:
    ReflectMembers();
};
