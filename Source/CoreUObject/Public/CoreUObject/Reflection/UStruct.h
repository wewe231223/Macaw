#pragma once

#include "CoreUObject/Reflection/FProperty.h"

class UStruct {
public:
    UStruct(FString Name, std::size_t Size);
    virtual ~UStruct();

public:
    const FString& GetName() const;
    std::size_t GetSize() const;

    void AddProperty(FProperty Property);
    virtual void Seal();

    const TArray<const FProperty*>& GetProperties() const;
    const FProperty* FindProperty(const FString& Name) const;
    bool ContainsProperty(const FProperty& Property) const;

    void SerializeProperties(void* Container, FArchive& Archive) const;

private:
    FString mName{};
    std::size_t mSize{};

    TArray<FProperty> mDeclaredProperties{};
    TArray<const FProperty*> mProperties{};
    bool mSealed{};
};
