#include "CoreUObject/Reflection/RegisterCoreUObject.h"
#include "CoreUObject/UObject.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FTransform.h"

void ReflectionModules::RegisterCoreUObject() {
    FVector::StaticStruct();
    FRotator::StaticStruct();
    FTransform::StaticStruct();

    UObject::StaticClass();
}
