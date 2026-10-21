#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/FReflectionBuilder.h"

void RegisterProperty(FReflectionBuilder& Owner, std::unique_ptr<FProperty> Property) {
    Owner.AddProperty(std::move(Property));
}
