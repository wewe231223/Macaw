#include "pch.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Core/Base/ErrorHandler.h"

UField::UField(FName FieldName)
	: mFieldName{FieldName} {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }

    SetName(FieldName);
}

FName UField::GetFieldName() const {
    return mFieldName;
}

UStruct::UStruct(FName FieldName, std::size_t Size, std::size_t Alignment)
	: UField{FieldName},
	  mStructureSize{Size},
	  mStructureAlignment{Alignment} {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

UStruct* UStruct::GetSuperStruct() const {
    return mSuperStruct;
}

void UStruct::SetSuperStruct(UStruct* SuperStruct) {
    if (SuperStruct != nullptr && SuperStruct->IsChildOf(this)) {
        ErrorHandler::Report("UStruct", "A reflected inheritance cycle is not allowed", ErrorHandler::EErrorLevel::Critical);
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
        ErrorHandler::Report("UStruct", "Invalid or duplicate reflected property", ErrorHandler::EErrorLevel::Critical);
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

void UStruct::SerializeProperties(void* Container, FArchive& Archive, bool IncludeInherited) const {
    const TArray<const FProperty*> Properties{GetProperties(IncludeInherited)};
    TArray<FString> Scopes{};

    for (const FProperty* Property : Properties) {
        if (Property->HasAnyFlags(EPropertyFlags::Serialize) && !Property->HasAnyFlags(EPropertyFlags::Transient) && std::ranges::find(Scopes, Property->GetSerializationScope()) == Scopes.end()) {
            Scopes.push_back(Property->GetSerializationScope());
        }
    }

    for (const FString& Scope : Scopes) {
        if (!Scope.empty()) {
            Archive.BeginObjectScope(Scope);
        }

        for (const FProperty* Property : Properties) {
            if (Property->HasAnyFlags(EPropertyFlags::Serialize) && !Property->HasAnyFlags(EPropertyFlags::Transient) && Property->GetSerializationScope() == Scope) {
                Property->SerializeContainer(Container, Archive);
            }
        }

        if (!Scope.empty()) {
            Archive.EndObjectScope();
        }
    }
}

void UStruct::SerializeDeclaredProperties(void* Container, FArchive& Archive) const {
    SerializeProperties(Container, Archive, false);
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
	: UStruct{FieldName, ValueType->mSize, ValueType->mAlignment},
	  mValueType{ValueType} {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
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
	: UField{FieldName} {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

void UEnum::AddValue(FName Name, Int64 Value) {
    if (FindValue(Name).has_value()) {
        ErrorHandler::Report("UEnum", "Duplicate reflected enum name", ErrorHandler::EErrorLevel::Critical);
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
