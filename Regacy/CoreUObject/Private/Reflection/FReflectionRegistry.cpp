#include "pch.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Core/Base/ErrorHandler.h"
#include <mutex>

namespace {
    enum class EDefinitionKind {
        Class,
        Struct,
        Enum
    };

    struct FDefinition {
        EDefinitionKind mKind{};

        UClass* (*mParent)(){};

        std::size_t mSize{};
        std::size_t mAlignment{};
        EClassFlags mFlags{};
        UClass::FFactory mFactory{};
        FReflectionRegistry::FClassBuilder mClassBuilder{};
        const FNativeType* mValueType{};
        FReflectionRegistry::FStructBuilder mStructBuilder{};
        FReflectionRegistry::FEnumBuilder mEnumBuilder{};
    };

    struct FRegistryState {
        std::recursive_mutex mMutex{};
        TMap<FName, std::unique_ptr<UField>> mFields{};
        TMap<FName, FDefinition> mDefinitions{};
    };

    FRegistryState& GetRegistryState() {
        static FRegistryState* State{new FRegistryState{}};

        return *State;
    }

    thread_local Uint32 BootstrapDepth{};

    class FBootstrapScope {
    public:
        FBootstrapScope();
        ~FBootstrapScope();
    };

    FBootstrapScope::FBootstrapScope() {
        ++BootstrapDepth;
    }

    FBootstrapScope::~FBootstrapScope() {
        --BootstrapDepth;
    }
}

UClass* FReflectionRegistry::DefineClass(FName Name, UClass* (*Parent)(), std::size_t Size, std::size_t Alignment, EClassFlags Flags, UClass::FFactory Factory, FClassBuilder Builder) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    if (Existing != State.mFields.end()) {
        const FDefinition& Definition{State.mDefinitions.at(Name)};

        if (Definition.mKind != EDefinitionKind::Class || Definition.mParent != Parent || Definition.mSize != Size || Definition.mAlignment != Alignment || Definition.mFlags != Flags || Definition.mFactory != Factory || Definition.mClassBuilder != Builder) {
            ErrorHandler::Report("FReflectionRegistry", "Conflicting reflected class name", ErrorHandler::EErrorLevel::Critical);
        }

        return static_cast<UClass*>(Existing->second.get());
    }

    const FBootstrapScope Bootstrap{};
    std::unique_ptr<UClass> Owner{std::make_unique<UClass>(Name, Size, Alignment, Flags, Factory)};
    UClass* Class{Owner.get()};

    State.mDefinitions.emplace(Name, FDefinition{EDefinitionKind::Class, Parent, Size, Alignment, Flags, Factory, Builder});
    State.mFields.emplace(Name, std::move(Owner));

    Class->SetSuperStruct(Parent != nullptr ? Parent() : nullptr);
    Class->InitializeClass(Name == FName{"UClass"} ? Class : UClass::StaticClass());

    if (Builder != nullptr) {
        Builder(*Class);
    }

    return Class;
}

UScriptStruct* FReflectionRegistry::DefineStruct(FName Name, const FNativeType* ValueType, FStructBuilder Builder) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    if (Existing != State.mFields.end()) {
        const FDefinition& Definition{State.mDefinitions.at(Name)};

        if (Definition.mKind != EDefinitionKind::Struct || Definition.mValueType != ValueType || Definition.mStructBuilder != Builder) {
            ErrorHandler::Report("FReflectionRegistry", "Conflicting reflected struct name", ErrorHandler::EErrorLevel::Critical);
        }

        return static_cast<UScriptStruct*>(Existing->second.get());
    }

    const FBootstrapScope Bootstrap{};
    std::unique_ptr<UScriptStruct> Owner{std::make_unique<UScriptStruct>(Name, ValueType)};
    UScriptStruct* Struct{Owner.get()};
    FDefinition Definition{};

    Definition.mKind = EDefinitionKind::Struct;
    Definition.mValueType = ValueType;
    Definition.mStructBuilder = Builder;
    State.mDefinitions.emplace(Name, Definition);
    State.mFields.emplace(Name, std::move(Owner));

    Struct->InitializeClass(UScriptStruct::StaticClass());

    if (Builder != nullptr) {
        Builder(*Struct);
    }

    return Struct;
}

UEnum* FReflectionRegistry::DefineEnum(FName Name, const FNativeType* ValueType, FEnumBuilder Builder) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    if (Existing != State.mFields.end()) {
        const FDefinition& Definition{State.mDefinitions.at(Name)};

        if (Definition.mKind != EDefinitionKind::Enum || Definition.mValueType != ValueType || Definition.mEnumBuilder != Builder) {
            ErrorHandler::Report("FReflectionRegistry", "Conflicting reflected enum name", ErrorHandler::EErrorLevel::Critical);
        }

        return static_cast<UEnum*>(Existing->second.get());
    }

    const FBootstrapScope Bootstrap{};
    std::unique_ptr<UEnum> Owner{std::make_unique<UEnum>(Name)};
    UEnum* Enum{Owner.get()};
    FDefinition Definition{};

    Definition.mKind = EDefinitionKind::Enum;
    Definition.mValueType = ValueType;
    Definition.mEnumBuilder = Builder;
    State.mDefinitions.emplace(Name, Definition);
    State.mFields.emplace(Name, std::move(Owner));

    Enum->InitializeClass(UEnum::StaticClass());

    if (Builder != nullptr) {
        Builder(*Enum);
    }

    return Enum;
}

void FReflectionRegistry::AddEnumValues(UEnum& Information, std::initializer_list<std::pair<FName, Int64>> Values) {
    for (const auto& [Name, Value] : Values) {
        Information.AddValue(Name, Value);
    }
}

UClass* FReflectionRegistry::FindClass(FName Name) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    return Existing != State.mFields.end() && State.mDefinitions.at(Name).mKind == EDefinitionKind::Class ? static_cast<UClass*>(Existing->second.get()) : nullptr;
}

UScriptStruct* FReflectionRegistry::FindStruct(FName Name) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    return Existing != State.mFields.end() && State.mDefinitions.at(Name).mKind == EDefinitionKind::Struct ? static_cast<UScriptStruct*>(Existing->second.get()) : nullptr;
}

UEnum* FReflectionRegistry::FindEnum(FName Name) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    const auto Existing{State.mFields.find(Name)};

    return Existing != State.mFields.end() && State.mDefinitions.at(Name).mKind == EDefinitionKind::Enum ? static_cast<UEnum*>(Existing->second.get()) : nullptr;
}

UEnum* FReflectionRegistry::FindEnum(const FNativeType* ValueType) {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};

    for (const auto& [Name, Definition] : State.mDefinitions) {
        if (Definition.mKind == EDefinitionKind::Enum && Definition.mValueType == ValueType) {
            return static_cast<UEnum*>(State.mFields.at(Name).get());
        }
    }

    return nullptr;
}

TArray<const UClass*> FReflectionRegistry::GetClasses() {
    FRegistryState& State{GetRegistryState()};
    const std::lock_guard<std::recursive_mutex> Lock{State.mMutex};
    TArray<const UClass*> Classes{};

    for (const auto& [Name, Field] : State.mFields) {
        if (State.mDefinitions.at(Name).mKind == EDefinitionKind::Class) {
            Classes.push_back(static_cast<const UClass*>(Field.get()));
        }
    }

    std::ranges::sort(Classes, [](const UClass* Left, const UClass* Right) {
        return Left->GetClassName().ToString() < Right->GetClassName().ToString();
    });

    return Classes;
}

bool FReflectionRegistry::IsBootstrapping() {
    return BootstrapDepth != 0;
}
