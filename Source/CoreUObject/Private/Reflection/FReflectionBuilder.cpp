#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"

FReflectionBuilder::FReflectionBuilder(UStruct& Owner)
	: mOwner{&Owner} {
}

UStruct* FReflectionBuilder::GetOwner() const {
    return mOwner;
}

void FReflectionBuilder::AddProperty(std::unique_ptr<FProperty> Property) {
    mOwner->AddProperty(std::move(*Property));
}
