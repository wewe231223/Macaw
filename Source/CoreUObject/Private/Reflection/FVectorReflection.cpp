#include "FVectorReflection.h"
#include "CoreUObject/Reflection/EPropertyFlags.h"
#include "Core/Math/FVector.h"
#include "CoreUObject/Reflection/ReflectionMacros.h"

ImplementStructReflection(FVector) {
    ReflectProperty(mX, "X", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mY, "Y", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    ReflectProperty(mZ, "Z", EPropertyFlags::Edit | EPropertyFlags::Serialize);
}
