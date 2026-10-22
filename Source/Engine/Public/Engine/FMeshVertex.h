#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

struct FMeshVertex {
public:
    ReflectTypes(FMeshVertex);

public:
    ReflectStruct(FMeshVertex);

private:
    ReflectMembers();

public:
    FVector mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FVector mNormal{};
    ReflectProperty(mNormal, "Normal", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
