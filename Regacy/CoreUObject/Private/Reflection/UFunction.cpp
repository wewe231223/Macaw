#include "pch.h"
#include "CoreUObject/Reflection/UFunction.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

UFunction::UFunction(FName Name, UClass* Owner, FInvoker Invoker)
	: UStruct(Name),
	  mOwnerClass(Owner),
	  mInvoker(Invoker) {
    InitializeClass(StaticClass());
    SetOuter(Owner);
}

void UFunction::AddParameter(FName Name, const FNativeType* Type) {
    AddProperty(std::make_unique<FProperty>(Name, this, Type, EPropertyFlags::Parameter));
    mParameterTypes.push_back(Type);
}

void UFunction::SetReturnType(const FNativeType* Type) {
    mReturnType = Type;

    if (Type != nullptr) {
        AddProperty(std::make_unique<FProperty>(FName{"ReturnValue"}, this, Type, EPropertyFlags::Return));
    }
}

const FNativeType* UFunction::GetReturnType() const {
    return mReturnType;
}

bool UFunction::Invoke(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) const {
    Result.Reset();

    if (!Object.IsA(mOwnerClass) || mInvoker == nullptr || Arguments.size() != mParameterTypes.size()) {
        return false;
    }

    for (std::size_t Index{}; Index < Arguments.size(); ++Index) {
        if (Arguments[Index].GetType() != mParameterTypes[Index]) {
            return false;
        }
    }

    if (!mInvoker(Object, Arguments, Result)) {
        Result.Reset();
        return false;
    }

    if (Result.GetType() != mReturnType) {
        Result.Reset();
        return false;
    }

    return true;
}
