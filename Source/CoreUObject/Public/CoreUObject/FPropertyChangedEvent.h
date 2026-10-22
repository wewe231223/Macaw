#pragma once

#include "Core/ReflectionMacros.h"

class FProperty;

struct FPropertyChangedEvent {
public:
    ReflectTypes(FPropertyChangedEvent);

public:
    ReflectStruct(FPropertyChangedEvent);

private:
    ReflectMembers();

public:
    const FProperty* mProperty{};
    ReflectProperty(mProperty, "Property", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mInteractive{};
    ReflectProperty(mInteractive, "Interactive", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
