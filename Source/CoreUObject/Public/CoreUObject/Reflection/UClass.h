#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/EClassFlags.h"

class UObject;

class UClass final : public UStruct {
public:
    ReflectTypes(UClass, UStruct);

public:
    using FFactory = std::unique_ptr<UObject> (*)();

public:
    UClass(FString Name, std::size_t Size, const UClass* Parent, FFactory Factory, EClassFlags Flags = EClassFlags::None);
    ~UClass() override;

public:
    ReflectStruct(UClass);

    const UClass* GetSuperClass() const;
    bool IsChildOf(const UClass* Parent) const;

    bool HasAnyFlags(EClassFlags Flags) const;
    bool CanCreate() const;
    std::unique_ptr<UObject> CreateInstance() const;

    void Seal() override;
    const TArray<const FProperty*>& GetAllProperties() const;
    const FProperty* FindClassProperty(const FString& Name) const;

    void SerializeClassProperties(void* Container, FArchive& Archive) const;

private:
    ReflectMembers();

private:
    const UClass* mParent{};
    ReflectProperty(mParent, "Parent", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    FFactory mFactory{};
    ReflectProperty(mFactory, "Factory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    EClassFlags mFlags{};
    ReflectProperty(mFlags, "Flags", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    TArray<const FProperty*> mAllProperties{};
    ReflectProperty(mAllProperties, "AllProperties", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mClassSealed{};
    ReflectProperty(mClassSealed, "ClassSealed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
