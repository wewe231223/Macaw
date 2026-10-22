#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "FRegisteredType.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FNativeType.h"
#include "Core/Base/ErrorHandler.h"

namespace {
    TMap<FString, FRegisteredType>& GetRegisteredTypes() {
        static TMap<FString, FRegisteredType> Types{};
        return Types;
    }

    UStruct* FindDefinition(const FString& Name, const FReflectionDefinition& Definition) {
        const auto Position{GetRegisteredTypes().find(Name)};

        if (Position == GetRegisteredTypes().end()) {
            return nullptr;
        }

        if (!(Position->second.mDefinition == Definition)) {
            ErrorHandler::Fatal("FReflectionRegistry", "Conflicting reflected type: " + Name);
        }

        return Position->second.mInformation.get();
    }

    UStruct* FindType(const FString& Name, EReflectionType Kind) {
        const auto Position{GetRegisteredTypes().find(Name)};
        return Position != GetRegisteredTypes().end() && Position->second.mDefinition.mKind == Kind ? Position->second.mInformation.get() : nullptr;
    }
}

UClass* FReflectionRegistry::DefineClass(FString Name, UClass* (*Parent)(), const FNativeType* NativeType, EClassFlags Flags, FFactory Factory, FClassBuilder Builder) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid class definition", ErrorHandler::EErrorLevel::Critical);
    const FReflectionDefinition Definition{EReflectionType::Class, NativeType, Parent, Flags, Factory, Builder};

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UClass*>(Existing);
    }

    UClass* Super{Parent != nullptr ? Parent() : nullptr};
    auto Owner{std::make_unique<UClass>(Name, NativeType->mSize, Super, Factory, Flags)};
    UClass* Class{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(*Class);
    }

    Class->Seal();

    return Class;
}

UScriptStruct* FReflectionRegistry::DefineStruct(FString Name, const FNativeType* NativeType, FStructBuilder Builder, const UScriptStruct* Parent) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid struct definition", ErrorHandler::EErrorLevel::Critical);
    FReflectionDefinition Definition{EReflectionType::Struct, NativeType};
    Definition.mStructBuilder = Builder;
    Definition.mStructParent = Parent;

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UScriptStruct*>(Existing);
    }

    auto Owner{std::make_unique<UScriptStruct>(Name, NativeType->mSize, Parent)};
    UScriptStruct* Struct{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(*Struct);
    }

    Struct->Seal();

    return Struct;
}

UEnum* FReflectionRegistry::DefineEnum(FString Name, const FNativeType* NativeType, FEnumBuilder Builder) {
    ErrorHandler::Report(Name.empty() || NativeType == nullptr, "FReflectionRegistry", "Invalid enum definition", ErrorHandler::EErrorLevel::Critical);
    FReflectionDefinition Definition{EReflectionType::Enum, NativeType};
    Definition.mEnumBuilder = Builder;

    if (UStruct* Existing{FindDefinition(Name, Definition)}) {
        return static_cast<UEnum*>(Existing);
    }

    auto Owner{std::make_unique<UEnum>(Name, NativeType->mSize)};
    UEnum* Enum{Owner.get()};

    GetRegisteredTypes().emplace(std::move(Name), FRegisteredType{Definition, std::move(Owner)});

    if (Builder != nullptr) {
        Builder(*Enum);
    }

    Enum->Seal();

    return Enum;
}

void FReflectionRegistry::AddEnumValues(UEnum& Information, std::initializer_list<FEnumValue> Values) {
    Information.AddValues(Values);
}

UClass* FReflectionRegistry::FindClass(const FString& Name) {
    return static_cast<UClass*>(FindType(Name, EReflectionType::Class));
}

UScriptStruct* FReflectionRegistry::FindStruct(const FString& Name) {
    return static_cast<UScriptStruct*>(FindType(Name, EReflectionType::Struct));
}

UEnum* FReflectionRegistry::FindEnum(const FString& Name) {
    return static_cast<UEnum*>(FindType(Name, EReflectionType::Enum));
}
