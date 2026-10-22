#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FVector.h"

struct FWorldHit {
public:
    ReflectTypes(FWorldHit);

public:
    ReflectStruct(FWorldHit);

private:
    ReflectMembers();

public:
    FObjectHandle mActor{};
    ReflectProperty(mActor, "Actor", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FObjectHandle mComponent{};
    ReflectProperty(mComponent, "Component", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    FVector mPosition{};
    ReflectProperty(mPosition, "Position", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    float mDistance{};
    ReflectProperty(mDistance, "Distance", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
