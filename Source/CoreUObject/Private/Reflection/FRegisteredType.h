#pragma once

#include "Core/ReflectionMacros.h"
#include "FReflectionDefinition.h"

struct FRegisteredType {
public:
    ReflectTypes(FRegisteredType);

public:
    ReflectStruct(FRegisteredType);

private:
    ReflectMembers();

public:
    FReflectionDefinition mDefinition{};
    ReflectProperty(mDefinition, "Definition", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::unique_ptr<UStruct> mInformation{};
    ReflectProperty(mInformation, "Information", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
