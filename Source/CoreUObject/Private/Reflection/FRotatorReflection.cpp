#include "FRotatorReflection.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "Core/Math/FRotator.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

ImplementStructReflection(FRotator) {
    ReflectProperty(mPitch, "Pitch", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mYaw, "Yaw", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mRoll, "Roll", EPropertyFlags::Edit | EPropertyFlags::Serialize);
}
