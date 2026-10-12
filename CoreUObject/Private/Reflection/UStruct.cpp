#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include <stdexcept>

UField::UField(FName FieldName)
	: mFieldName(FieldName) {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }

    SetName(FieldName);
}

UClass* UField::StaticClass() {
    return FReflectionRegistry::DefineClass(FName{"UField"}, &UObject::StaticClass, sizeof(UField), alignof(UField), EClassFlags::Metadata | EClassFlags::NoFactory, nullptr);
}

FName UField::GetFieldName() const {
    return mFieldName;
}

UStruct::UStruct(FName FieldName, std::size_t Size, std::size_t Alignment)
	: UField(FieldName),
	  mStructureSize(Size),
	  mStructureAlignment(Alignment) {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

UClass* UStruct::StaticClass() {
    return FReflectionRegistry::DefineClass(FName{"UStruct"}, &UField::StaticClass, sizeof(UStruct), alignof(UStruct), EClassFlags::Metadata | EClassFlags::NoFactory, nullptr);
}

UStruct* UStruct::GetSuperStruct() const {
    return mSuperStruct;
}

void UStruct::SetSuperStruct(UStruct* SuperStruct) {
    if (SuperStruct != nullptr && SuperStruct->IsChildOf(this)) {
        throw std::logic_error{"A reflected inheritance cycle is not allowed"};
    }

    mSuperStruct = SuperStruct;
}

bool UStruct::IsChildOf(const UStruct* BaseStruct) const {
    for (const UStruct* Current{this}; Current != nullptr; Current = Current->mSuperStruct) {
        if (Current == BaseStruct) {
            return true;
        }
    }

    return false;
}

std::size_t UStruct::GetStructureSize() const {
    return mStructureSize;
}

std::size_t UStruct::GetStructureAlignment() const {
    return mStructureAlignment;
}

void UStruct::AddProperty(std::unique_ptr<FProperty> Property) {
    if (Property == nullptr || Property->GetOwner() != this || FindProperty(Property->GetName()) != nullptr) {
        throw std::logic_error{"Invalid or duplicate reflected property"};
    }

    mProperties.push_back(std::move(Property));
}

const TArray<std::unique_ptr<FProperty>>& UStruct::GetDeclaredProperties() const {
    return mProperties;
}

TArray<const FProperty*> UStruct::GetProperties(bool IncludeInherited) const {
    TArray<const FProperty*> Properties{};

    if (IncludeInherited && mSuperStruct != nullptr) {
        Properties = mSuperStruct->GetProperties(true);
    }

    for (const std::unique_ptr<FProperty>& Property : mProperties) {
        Properties.push_back(Property.get());
    }

    return Properties;
}

const FProperty* UStruct::FindProperty(FName Name) const {
    for (const std::unique_ptr<FProperty>& Property : mProperties) {
        if (Property->GetName() == Name) {
            return Property.get();
        }
    }

    return mSuperStruct != nullptr ? mSuperStruct->FindProperty(Name) : nullptr;
}

void UStruct::SerializeDeclaredProperties(void* Container, FArchive& Archive) const {
    for (const std::unique_ptr<FProperty>& Property : mProperties) {
        if (Property->HasAnyFlags(EPropertyFlags::Serialize) && !Property->HasAnyFlags(EPropertyFlags::Transient)) {
            Property->SerializeContainer(Container, Archive);
        }
    }
}

void UStruct::VisitObjectReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const {
    if (mSuperStruct != nullptr) {
        mSuperStruct->VisitObjectReferences(Container, Visitor);
    }

    for (const std::unique_ptr<FProperty>& Property : mProperties) {
        Property->VisitReferences(Container, Visitor);
    }
}

UScriptStruct::UScriptStruct(FName FieldName, const FNativeType* ValueType)
	: UStruct(FieldName, ValueType->mSize, ValueType->mAlignment),
	  mValueType(ValueType) {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

UClass* UScriptStruct::StaticClass() {
    return FReflectionRegistry::DefineClass(FName{"UScriptStruct"}, &UStruct::StaticClass, sizeof(UScriptStruct), alignof(UScriptStruct), EClassFlags::Metadata | EClassFlags::NoFactory, nullptr);
}

const FNativeType* UScriptStruct::GetValueType() const {
    return mValueType;
}

void VisitScriptStructReferences(const UScriptStruct* Struct, const void* Value, const std::function<void(UObject*)>& Visitor) {
    if (Struct != nullptr) {
        Struct->VisitObjectReferences(Value, Visitor);
    }
}

UEnum::UEnum(FName FieldName)
	: UField(FieldName) {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

UClass* UEnum::StaticClass() {
    return FReflectionRegistry::DefineClass(FName{"UEnum"}, &UField::StaticClass, sizeof(UEnum), alignof(UEnum), EClassFlags::Metadata | EClassFlags::NoFactory, nullptr);
}

void UEnum::AddValue(FName Name, Int64 Value) {
    if (FindValue(Name).has_value()) {
        throw std::logic_error{"Duplicate reflected enum name"};
    }

    mValues.emplace_back(Name, Value);
}

std::optional<Int64> UEnum::FindValue(FName Name) const {
    for (const auto& [EntryName, Value] : mValues) {
        if (EntryName == Name) {
            return Value;
        }
    }

    return std::nullopt;
}

FName UEnum::FindName(Int64 Value) const {
    for (const auto& [EntryName, EntryValue] : mValues) {
        if (EntryValue == Value) {
            return EntryName;
        }
    }

    return {};
}

const TArray<std::pair<FName, Int64>>& UEnum::GetValues() const {
    return mValues;
}
