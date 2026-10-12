#pragma once

#include "CoreUObject/Reflection/FPropertyValue.h"
#include <limits>

class FArchive;
class UStruct;
class UEnum;

enum class EPropertyFlags : Uint32 {
    None = 0,
    Edit = 1,
    Serialize = 2,
    ReadOnly = 4,
    Transient = 8,
    Parameter = 16,
    Return = 32,
    Reference = 64,
    Owned = 128,
    Color = 256
};

EPropertyFlags operator|(EPropertyFlags Left, EPropertyFlags Right);

class FField {
public:
    FField(FName Name, UStruct* Owner);
    virtual ~FField() = default;

public:
    FName GetName() const;
    UStruct* GetOwner() const;

private:
    FName mFieldName{};
    UStruct* mOwner{};
};

class FProperty : public FField {
public:
    using FReader = FPropertyValue (*)(const void*);
    using FWriter = bool (*)(void*, const FPropertyValue&);
    using FSerializer = void (*)(void*, FArchive&);
    using FReferenceVisitor = void (*)(const void*, const std::function<void(UObject*)>&);

public:
    FProperty(FName Name, UStruct* Owner, const FNativeType* Type, EPropertyFlags Flags, FReader Reader = nullptr, FWriter Writer = nullptr, FSerializer Serializer = nullptr, FReferenceVisitor ReferenceVisitor = nullptr);
    ~FProperty() override = default;

public:
    const FNativeType* GetValueType() const;
    EPropertyKind GetKind() const;
    UScriptStruct* GetStruct() const;
    UEnum* GetEnum() const;
    UClass* GetObjectClass() const;
    const FNativeType* GetElementType() const;
    bool HasAnyFlags(EPropertyFlags Flags) const;
    bool CanWrite() const;
    FPropertyValue GetValue(const UObject& Object) const;
    bool SetValue(UObject& Object, const FPropertyValue& Value) const;
    FPropertyValue ReadContainer(const void* Container) const;
    bool WriteContainer(void* Container, const FPropertyValue& Value) const;
    void SerializeContainer(void* Container, FArchive& Archive) const;
    void VisitReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const;
    bool ResetToDefault(UObject& Object) const;
    void SetEditorMetadata(FString Category, FString DisplayName, float Minimum, float Maximum, float Speed);
    const FString& GetCategory() const;
    const FString& GetDisplayName() const;
    float GetMinimum() const;
    float GetMaximum() const;
    float GetSpeed() const;

private:
    const FNativeType* mValueType{};
    EPropertyFlags mFlags{};
    FReader mReader{};
    FWriter mWriter{};
    FSerializer mSerializer{};
    FReferenceVisitor mReferenceVisitor{};
    FString mCategory{};
    FString mDisplayName{};
    float mMinimum{-std::numeric_limits<float>::max()};
    float mMaximum{std::numeric_limits<float>::max()};
    float mSpeed{0.1f};
};
