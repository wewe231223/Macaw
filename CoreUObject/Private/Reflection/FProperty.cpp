#include "pch.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include <cmath>
#include "CoreUObject/Reflection/NativeSerialization.h"

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
	  mDisplayName(Name.ToString()),
	  mSerializationName(Name.ToString()) {
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

void FProperty::SetSerializer(FSerializer Serializer) {
    mSerializer = Serializer;
}

void FProperty::SerializeContainer(void* Container, FArchive& Archive) const {
    if (Container != nullptr && mSerializer != nullptr) {
        mSerializer(Container, Archive, *this);
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

void FProperty::SetEditorMetadata(FString Category, FString DisplayName, double Minimum, double Maximum, float Speed) {
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

double FProperty::GetMinimum(const void* Container) const {
    const double Value{Container != nullptr && mMinimumGetter != nullptr ? mMinimumGetter(Container) : mMinimum};

    return std::isfinite(Value) ? (std::max)(mMinimum, Value) : mMinimum;
}

double FProperty::GetMaximum(const void* Container) const {
    const double Value{Container != nullptr && mMaximumGetter != nullptr ? mMaximumGetter(Container) : mMaximum};

    return std::isfinite(Value) ? (std::min)(mMaximum, Value) : mMaximum;
}

float FProperty::GetSpeed() const {
    return mSpeed;
}

void FProperty::SetDynamicBounds(FBoundGetter Minimum, FBoundGetter Maximum) {
    mMinimumGetter = Minimum;
    mMaximumGetter = Maximum;
}

void FProperty::SetEditCondition(FEditCondition Condition) {
    mEditCondition = Condition;
}

bool FProperty::CanEdit(const void* Container) const {
    return HasAnyFlags(EPropertyFlags::Edit) && CanWrite() && Container != nullptr && (mEditCondition == nullptr || mEditCondition(Container));
}

void FProperty::SetOptionsGetter(FOptionsGetter Getter) {
    mOptionsGetter = Getter;
}

bool FProperty::HasOptions() const {
    return mOptionsGetter != nullptr;
}

TArray<FName> FProperty::GetOptions(const void* Container) const {
    return Container != nullptr && mOptionsGetter != nullptr ? mOptionsGetter(Container) : TArray<FName>{};
}

void FProperty::SetAssetClass(UClass* AssetClass) {
    mAssetClass = AssetClass;
}

UClass* FProperty::GetAssetClass() const {
    return mAssetClass;
}

UObject* FProperty::ReadObject(const void* Container) const {
    const FPropertyValue Value{ReadContainer(Container)};

    return !Value.IsEmpty() && mValueType->mReadObject != nullptr ? mValueType->mReadObject(Value.GetData()) : nullptr;
}

FPropertyValue FProperty::MakeObjectValue(UObject* Object) const {
    return mValueType != nullptr && mValueType->mMakeObjectValue != nullptr ? mValueType->mMakeObjectValue(Object) : FPropertyValue{};
}

void FProperty::SetSerializationScope(FString Scope) {
    mSerializationScope = std::move(Scope);
}

const FString& FProperty::GetSerializationScope() const {
    return mSerializationScope;
}

void SerializeAssetReference(FArchive& Archive, std::string_view GuidName, std::string_view PathName, FAssetHandle& Handle, FGuid& Guid, FAssetPath& Path) {
    const IAssetResolver* Resolver{Archive.GetAssetResolver()};

    if (!Archive.IsLoading() && Resolver != nullptr) {
        if (const FGuid* PersistentGuid{Resolver->GetAssetGuid(Handle)}) {
            Guid = *PersistentGuid;
        }

        if (const FAssetPath* PersistentPath{Resolver->GetAssetPath(Handle)}) {
            Path = *PersistentPath;
        }
    }

    Archive.Serialize(GuidName, Guid);
    Archive.Serialize(PathName, Path.mPath);

    if (Archive.IsLoading()) {
        Handle = Resolver != nullptr ? Resolver->FindAsset(Guid) : FAssetHandle{};

        if (!Handle && Resolver != nullptr) {
            Handle = Resolver->FindAsset(Path);
        }
    }
}

void SerializeObjectReference(FArchive& Archive, std::string_view Name, const UObject* Object, FGuid& PendingGuid) {
    FString Guid{Archive.IsLoading() ? FString{} : Object != nullptr ? Object->GetGuid().ToString() : PendingGuid.IsValid() ? PendingGuid.ToString() : FString{}};

    Archive.Serialize(Name, Guid);

    if (Archive.IsLoading()) {
        PendingGuid = {};

        if (!Guid.empty() && !PendingGuid.Parse(Guid)) {
            Archive.SetError();
            return;
        }

        if (!Archive.HasError()) {
            Archive.RemapObjectReference(PendingGuid);
        }
    }
}

void FProperty::SetObjectFilter(FObjectFilter Filter) {
    mObjectFilter = Filter;
}

bool FProperty::CanSelectObject(const void* Container, UObject* Object) const {
    const UClass* Class{GetObjectClass()};

    return Container != nullptr && Class != nullptr && (Object == nullptr || Object->IsA(Class)) && (mObjectFilter == nullptr || mObjectFilter(Container, Object));
}

void FProperty::SetSerializationName(FString Name) {
    mSerializationName = std::move(Name);
}

const FString& FProperty::GetSerializationName() const {
    return mSerializationName;
}
