#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/EClassFlags.h"
#include <mutex>

class UFunction;

struct FObjectConstructionParameters {
    UObject* mOuter{};
    FName mName{};
    bool mRegister{};
};

class UClass : public UStruct {
public:
    using ReflectionOwner = UClass;
    using Super = UStruct;
    using UStruct::IsChildOf;
    using FFactory = std::unique_ptr<UObject> (*)();

public:
    UClass(FName ClassName, std::size_t Size, std::size_t Alignment, EClassFlags Flags, FFactory Factory);
    ~UClass() override;

public:
    ReflectClass(UClass, EClassFlags::Metadata | EClassFlags::NoFactory);
    FName GetClassName() const;
    UClass* GetSuperClass() const;
    bool IsChildOf(const UClass* BaseClass) const;
    template <typename T>
    bool IsChildOf() const;
    bool HasAnyFlags(EClassFlags Flags) const;
    bool CanCreateInstance() const;
    std::unique_ptr<UObject> CreateInstance(const FObjectConstructionParameters& Parameters = {}) const;
    const UObject* GetDefaultObject() const;
    bool ResetToDefault(UObject& Object) const;
    void AddFunction(std::unique_ptr<UFunction> Function);
    const UFunction* FindFunction(FName Name) const;
    TArray<const UFunction*> GetFunctions(bool IncludeInherited = true) const;

private:
    EClassFlags mClassFlags{};
    FFactory mFactory{};
    mutable std::recursive_mutex mDefaultMutex{};
    mutable bool mCreatingDefault{};
    mutable std::unique_ptr<UObject> mDefaultObject{};
    TArray<std::unique_ptr<UFunction>> mFunctions{};
};

template <typename T>
bool UClass::IsChildOf() const {
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return IsChildOf(T::StaticClass());
}
