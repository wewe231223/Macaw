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
    Color = 256,
    Asset = 512,
    Inline = 1024,
    OwnerOnly = 2048
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
    using FSerializer = void (*)(void*, FArchive&, const FProperty&);
    using FBoundGetter = double (*)(const void*);
    using FObjectFilter = bool (*)(const void*, UObject*);
    using FEditCondition = bool (*)(const void*);
    using FOptionsGetter = TArray<FName> (*)(const void*);
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
    void SetSerializer(FSerializer Serializer);
    void SerializeContainer(void* Container, FArchive& Archive) const;
    void VisitReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const;
    bool ResetToDefault(UObject& Object) const;
    void SetEditorMetadata(FString Category, FString DisplayName, double Minimum, double Maximum, float Speed);
    const FString& GetCategory() const;
    const FString& GetDisplayName() const;
    void SetDynamicBounds(FBoundGetter Minimum, FBoundGetter Maximum);
    void SetEditCondition(FEditCondition Condition);
    bool CanEdit(const void* Container) const;
    void SetOptionsGetter(FOptionsGetter Getter);
    bool HasOptions() const;
    TArray<FName> GetOptions(const void* Container) const;
    void SetAssetClass(UClass* AssetClass);
    UClass* GetAssetClass() const;
    UObject* ReadObject(const void* Container) const;
    FPropertyValue MakeObjectValue(UObject* Object) const;
    void SetObjectFilter(FObjectFilter Filter);
    bool CanSelectObject(const void* Container, UObject* Object) const;
    void SetSerializationName(FString Name);
    const FString& GetSerializationName() const;
    void SetSerializationScope(FString Scope);
    const FString& GetSerializationScope() const;
    double GetMinimum(const void* Container = nullptr) const;
    double GetMaximum(const void* Container = nullptr) const;
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
    double mMinimum{-std::numeric_limits<double>::max()};
    double mMaximum{std::numeric_limits<double>::max()};
    FBoundGetter mMinimumGetter{};
    FBoundGetter mMaximumGetter{};
    FEditCondition mEditCondition{};
    FObjectFilter mObjectFilter{};
    FOptionsGetter mOptionsGetter{};
    UClass* mAssetClass{};
    FString mSerializationName{};
    FString mSerializationScope{};
    float mSpeed{0.1f};
};
