#include "FTransformReflection.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

ImplementStructReflection(FTransform) {
    ReflectProperty(mLocation, "Location", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mRotation, "Rotation", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mScale, "Scale", EPropertyFlags::Edit | EPropertyFlags::Serialize);
}
