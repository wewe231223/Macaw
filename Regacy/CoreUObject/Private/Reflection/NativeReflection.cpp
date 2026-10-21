#include "pch.h"
#include "CoreUObject/Reflection/NativeReflection.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/UFunction.h"
#include "Core/Base/ErrorHandler.h"

FReflectionBuilder::FReflectionBuilder(UClass& Class)
	: mOwner{&Class},
	  mClass{&Class} {
}

FReflectionBuilder::FReflectionBuilder(UScriptStruct& Struct)
	: mOwner{&Struct} {
}

UStruct* FReflectionBuilder::GetOwner() const {
    return mOwner;
}

void FReflectionBuilder::AddProperty(std::unique_ptr<FProperty> Property) {
    mOwner->AddProperty(std::move(Property));
}

void FReflectionBuilder::AddFunction(FName Name, FInvoker Invoker, std::initializer_list<FName> ParameterNames, std::initializer_list<const FNativeType*> ParameterTypes, const FNativeType* ReturnType) {
    if (mClass == nullptr || ParameterNames.size() != ParameterTypes.size()) {
        ErrorHandler::Report("FReflectionBuilder", "Reflected functions require a class and matching parameter names", ErrorHandler::EErrorLevel::Critical);
    }

    std::unique_ptr<UFunction> Function{std::make_unique<UFunction>(Name, mClass, Invoker)};
    const FNativeType* const* Type{ParameterTypes.begin()};

    for (FName ParameterName : ParameterNames) {
        Function->AddParameter(ParameterName, *Type++);
    }

    if (ReturnType != nullptr) {
        Function->SetReturnType(ReturnType);
    }

    mClass->AddFunction(std::move(Function));
}

void SerializeScriptStructProperties(const UScriptStruct* Struct, void* Container, FArchive& Archive, bool IncludeInherited) {
    Struct->SerializeProperties(Container, Archive, IncludeInherited);
}
