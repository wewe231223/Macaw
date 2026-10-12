#include "pch.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"

EPropertyFlags operator|(EPropertyFlags Left, EPropertyFlags Right) {
    return static_cast<EPropertyFlags>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}

FField::FField(FName Name, UStruct* Owner)
	: mFieldName(Name),
	  mOwner(Owner) {
}

FName FField::GetName() const {
    return mFieldName;
}

UStruct* FField::GetOwner() const {
    return mOwner;
}

FProperty::FProperty(FName Name, UStruct* Owner, const FNativeType* Type, EPropertyFlags Flags, FReader Reader, FWriter Writer, FSerializer Serializer, FReferenceVisitor ReferenceVisitor)
	: FField(Name, Owner),
	  mValueType(Type),
	  mFlags(Flags),
	  mReader(Reader),
	  mWriter(Writer),
	  mSerializer(Serializer),
	  mReferenceVisitor(ReferenceVisitor),
	  mDisplayName(Name.ToString()) {
}

const FNativeType* FProperty::GetValueType() const {
    return mValueType;
}

EPropertyKind FProperty::GetKind() const {
    return mValueType != nullptr ? mValueType->mKind : EPropertyKind::Unknown;
}

UScriptStruct* FProperty::GetStruct() const {
    return mValueType != nullptr && mValueType->mGetStruct != nullptr ? mValueType->mGetStruct() : nullptr;
}

UEnum* FProperty::GetEnum() const {
    return FReflectionRegistry::FindEnum(mValueType);
}

UClass* FProperty::GetObjectClass() const {
    return mValueType != nullptr && mValueType->mGetObjectClass != nullptr ? mValueType->mGetObjectClass() : nullptr;
}

const FNativeType* FProperty::GetElementType() const {
    return mValueType != nullptr && mValueType->mGetElementType != nullptr ? mValueType->mGetElementType() : nullptr;
}

bool FProperty::HasAnyFlags(EPropertyFlags Flags) const {
    return (static_cast<Uint32>(mFlags) & static_cast<Uint32>(Flags)) != 0;
}

bool FProperty::CanWrite() const {
    return mWriter != nullptr && !HasAnyFlags(EPropertyFlags::ReadOnly);
}

FPropertyValue FProperty::GetValue(const UObject& Object) const {
    if (Object.GetClass() == nullptr || !Object.GetClass()->IsChildOf(GetOwner())) {
        return {};
    }

    return ReadContainer(&Object);
}

bool FProperty::SetValue(UObject& Object, const FPropertyValue& Value) const {
    return Object.GetClass() != nullptr && Object.GetClass()->IsChildOf(GetOwner()) && WriteContainer(&Object, Value);
}

FPropertyValue FProperty::ReadContainer(const void* Container) const {
    return Container != nullptr && mReader != nullptr ? mReader(Container) : FPropertyValue{};
}

bool FProperty::WriteContainer(void* Container, const FPropertyValue& Value) const {
    return Container != nullptr && CanWrite() && Value.GetType() == mValueType && mWriter(Container, Value);
}

void FProperty::SerializeContainer(void* Container, FArchive& Archive) const {
    if (Container != nullptr && mSerializer != nullptr) {
        mSerializer(Container, Archive);
    }
}

void FProperty::VisitReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const {
    if (Container != nullptr && mReferenceVisitor != nullptr) {
        mReferenceVisitor(Container, Visitor);
    }
}

bool FProperty::ResetToDefault(UObject& Object) const {
    const UObject* Default{Object.GetClass()->GetDefaultObject()};

    return Default != nullptr && SetValue(Object, GetValue(*Default));
}

void FProperty::SetEditorMetadata(FString Category, FString DisplayName, float Minimum, float Maximum, float Speed) {
    mCategory = std::move(Category);
    mDisplayName = std::move(DisplayName);
    mMinimum = Minimum;
    mMaximum = Maximum;
    mSpeed = Speed;
}

const FString& FProperty::GetCategory() const {
    return mCategory;
}

const FString& FProperty::GetDisplayName() const {
    return mDisplayName;
}

float FProperty::GetMinimum() const {
    return mMinimum;
}

float FProperty::GetMaximum() const {
    return mMaximum;
}

float FProperty::GetSpeed() const {
    return mSpeed;
}
