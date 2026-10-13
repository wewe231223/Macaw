#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UFunction.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "CoreUObject/UObjectSystem.h"
#include <stdexcept>

EClassFlags operator|(EClassFlags Left, EClassFlags Right) {
    return static_cast<EClassFlags>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}

UClass::UClass(FName ClassName, std::size_t Size, std::size_t Alignment, EClassFlags Flags, FFactory Factory)
	: UStruct(ClassName, Size, Alignment),
	  mClassFlags(Flags),
	  mFactory(Factory) {
    if (!FReflectionRegistry::IsBootstrapping()) {
        InitializeClass(StaticClass());
    }
}

UClass::~UClass() = default;

FName UClass::GetClassName() const {
    return GetFieldName();
}

UClass* UClass::GetSuperClass() const {
    return static_cast<UClass*>(GetSuperStruct());
}

bool UClass::IsChildOf(const UClass* BaseClass) const {
    return UStruct::IsChildOf(BaseClass);
}

bool UClass::HasAnyFlags(EClassFlags Flags) const {
    return (static_cast<Uint32>(mClassFlags) & static_cast<Uint32>(Flags)) != 0;
}

bool UClass::CanCreateInstance() const {
    return mFactory != nullptr && !HasAnyFlags(EClassFlags::Abstract | EClassFlags::NoFactory | EClassFlags::Metadata);
}

std::unique_ptr<UObject> UClass::CreateInstance(const FObjectConstructionParameters& Parameters) const {
    if (!CanCreateInstance()) {
        return nullptr;
    }

    const FObjectInitializer Initializer{const_cast<UClass*>(this)};
    std::unique_ptr<UObject> Object{mFactory()};

    if (Object == nullptr || Object->GetClass() != this || !Object->SetOuter(Parameters.mOuter) || (!Parameters.mName.IsNone() && !Object->SetName(Parameters.mName))) {
        return nullptr;
    }

    if (Parameters.mRegister && !UObjectSystem::Register(Object.get()).IsValid()) {
        return nullptr;
    }

    return Object;
}

const UObject* UClass::GetDefaultObject() const {
    const std::lock_guard<std::recursive_mutex> Lock{mDefaultMutex};

    if (mDefaultObject == nullptr && !mCreatingDefault && mFactory != nullptr && !HasAnyFlags(EClassFlags::NoFactory | EClassFlags::Metadata)) {
        mCreatingDefault = true;

        try {
            const FObjectInitializer Initializer{const_cast<UClass*>(this), true};
            mDefaultObject = mFactory();

            if (mDefaultObject != nullptr && mDefaultObject->GetClass() != this) {
                mDefaultObject.reset();
            }

            if (mDefaultObject != nullptr) {
                mDefaultObject->mClassDefaultObject = true;
            }
        } catch (...) {
            mCreatingDefault = false;
            throw;
        }

        mCreatingDefault = false;
    }

    return mDefaultObject.get();
}

bool UClass::ResetToDefault(UObject& Object) const {
    if (!Object.IsA(this) || GetDefaultObject() == nullptr) {
        return false;
    }

    for (const FProperty* Property : GetProperties()) {
        if (Property->CanWrite() && !Property->ResetToDefault(Object)) {
            return false;
        }
    }

    return true;
}

void UClass::AddFunction(std::unique_ptr<UFunction> Function) {
    if (Function == nullptr) {
        throw std::logic_error{"Cannot register an empty reflected function"};
    }

    for (const std::unique_ptr<UFunction>& Existing : mFunctions) {
        if (Existing->GetFieldName() == Function->GetFieldName()) {
            throw std::logic_error{"Duplicate reflected function"};
        }
    }

    mFunctions.push_back(std::move(Function));
}

const UFunction* UClass::FindFunction(FName Name) const {
    for (const std::unique_ptr<UFunction>& Function : mFunctions) {
        if (Function->GetFieldName() == Name) {
            return Function.get();
        }
    }

    return GetSuperClass() != nullptr ? GetSuperClass()->FindFunction(Name) : nullptr;
}

TArray<const UFunction*> UClass::GetFunctions(bool IncludeInherited) const {
    TArray<const UFunction*> Functions{};

    if (IncludeInherited && GetSuperClass() != nullptr) {
        Functions = GetSuperClass()->GetFunctions();
    }

    for (const std::unique_ptr<UFunction>& Function : mFunctions) {
        const auto Existing{std::ranges::find_if(Functions, [&Function](const UFunction* Candidate) {
            return Candidate->GetFieldName() == Function->GetFieldName();
        })};

        if (Existing != Functions.end()) {
            *Existing = Function.get();
        } else {
            Functions.push_back(Function.get());
        }
    }

    return Functions;
}
