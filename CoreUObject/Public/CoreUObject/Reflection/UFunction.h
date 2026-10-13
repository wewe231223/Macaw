#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/Reflection/UStruct.h"
#include <span>

class UFunction : public UStruct {
public:
    using ReflectionOwner = UFunction;
    using Super = UStruct;
    using FInvoker = bool (*)(UObject&, std::span<const FPropertyValue>, FPropertyValue&);

public:
    UFunction(FName Name, UClass* Owner, FInvoker Invoker);
    ~UFunction() override = default;

public:
    ReflectClass(UFunction, EClassFlags::Metadata | EClassFlags::NoFactory);
    void AddParameter(FName Name, const FNativeType* Type);
    void SetReturnType(const FNativeType* Type);
    const FNativeType* GetReturnType() const;
    bool Invoke(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) const;

private:
    UClass* mOwnerClass{};
    FInvoker mInvoker{};
    TArray<const FNativeType*> mParameterTypes{};
    const FNativeType* mReturnType{};
};
