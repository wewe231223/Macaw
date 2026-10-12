#pragma once

#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FProperty.h"

class UField : public UObject {
public:
    using ReflectionOwner = UField;
    using Super = UObject;

public:
    explicit UField(FName FieldName);
    ~UField() override = default;

public:
    static UClass* StaticClass();
    FName GetFieldName() const;

private:
    FName mFieldName{};
};

class UStruct : public UField {
public:
    using ReflectionOwner = UStruct;
    using Super = UField;

public:
    explicit UStruct(FName FieldName, std::size_t Size = 0, std::size_t Alignment = 1);
    ~UStruct() override = default;

public:
    static UClass* StaticClass();
    UStruct* GetSuperStruct() const;
    void SetSuperStruct(UStruct* SuperStruct);
    bool IsChildOf(const UStruct* BaseStruct) const;
    std::size_t GetStructureSize() const;
    std::size_t GetStructureAlignment() const;
    void AddProperty(std::unique_ptr<FProperty> Property);
    const TArray<std::unique_ptr<FProperty>>& GetDeclaredProperties() const;
    TArray<const FProperty*> GetProperties(bool IncludeInherited = true) const;
    const FProperty* FindProperty(FName Name) const;
    void SerializeProperties(void* Container, FArchive& Archive, bool IncludeInherited = true) const;
    void SerializeDeclaredProperties(void* Container, FArchive& Archive) const;
    void VisitObjectReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const;

private:
    UStruct* mSuperStruct{};
    std::size_t mStructureSize{};
    std::size_t mStructureAlignment{1};
    TArray<std::unique_ptr<FProperty>> mProperties{};
};

class UScriptStruct : public UStruct {
public:
    using ReflectionOwner = UScriptStruct;
    using Super = UStruct;

public:
    UScriptStruct(FName FieldName, const FNativeType* ValueType);
    ~UScriptStruct() override = default;

public:
    static UClass* StaticClass();
    const FNativeType* GetValueType() const;

private:
    const FNativeType* mValueType{};
};

class UEnum : public UField {
public:
    using ReflectionOwner = UEnum;
    using Super = UField;

public:
    explicit UEnum(FName FieldName);
    ~UEnum() override = default;

public:
    static UClass* StaticClass();
    void AddValue(FName Name, Int64 Value);
    std::optional<Int64> FindValue(FName Name) const;
    FName FindName(Int64 Value) const;
    const TArray<std::pair<FName, Int64>>& GetValues() const;

private:
    TArray<std::pair<FName, Int64>> mValues{};
};
