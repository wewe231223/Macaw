#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FProperty.h"

class UStruct {
public:
    ReflectTypes(UStruct);

public:
    UStruct(FString Name, std::size_t Size);
    virtual ~UStruct();

public:
    ReflectStruct(UStruct);

    const FString& GetName() const;
    std::size_t GetSize() const;

    void AddProperty(FProperty Property);
    virtual void Seal();

    const TArray<const FProperty*>& GetProperties() const;
    const FProperty* FindProperty(const FString& Name) const;
    bool ContainsProperty(const FProperty& Property) const;

    void SerializeProperties(void* Container, FArchive& Archive) const;

private:
    ReflectMembers();

private:
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    std::size_t mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    TArray<FProperty> mDeclaredProperties{};
    ReflectProperty(mDeclaredProperties, "DeclaredProperties", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<const FProperty*> mProperties{};
    ReflectProperty(mProperties, "Properties", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    bool mSealed{};
    ReflectProperty(mSealed, "Sealed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
