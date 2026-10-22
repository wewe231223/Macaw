#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

struct FEnumValue {
public:
    ReflectTypes(FEnumValue);

public:
    ReflectStruct(FEnumValue);

private:
    ReflectMembers();

public:
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    Int32 mValue{};
    ReflectProperty(mValue, "Value", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
