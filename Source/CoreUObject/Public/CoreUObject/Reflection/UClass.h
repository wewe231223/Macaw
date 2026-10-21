#pragma once

#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/EClassFlags.h"

class UObject;

class UClass final : public UStruct {
public:
    using FFactory = std::unique_ptr<UObject> (*)();

public:
    UClass(FString Name, std::size_t Size, const UClass* Parent, FFactory Factory, EClassFlags Flags = EClassFlags::None);
    ~UClass() override;

public:
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
    const UClass* mParent{};
    FFactory mFactory{};
    EClassFlags mFlags{};

    TArray<const FProperty*> mAllProperties{};
    bool mClassSealed{};
};
