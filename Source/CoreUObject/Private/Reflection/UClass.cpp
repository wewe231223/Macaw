#include "CoreUObject/Reflection/UClass.h"
#include "Core/Archive.h"
#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

UClass::UClass(FString Name, std::size_t Size, const UClass* Parent, FFactory Factory, EClassFlags Flags)
	: UStruct{std::move(Name), Size},
	  mParent{Parent},
	  mFactory{Factory},
	  mFlags{Flags} {
}

UClass::~UClass() = default;

const UClass* UClass::GetSuperClass() const {
    return mParent;
}

bool UClass::IsChildOf(const UClass* Parent) const {
    for (const UClass* Current{this}; Current != nullptr; Current = Current->mParent) {
        if (Current == Parent) {
            return true;
        }
    }

    return false;
}

bool UClass::HasAnyFlags(EClassFlags Flags) const {
    return (static_cast<Uint32>(mFlags) & static_cast<Uint32>(Flags)) != 0;
}

bool UClass::CanCreate() const {
    return mFactory != nullptr;
}

std::unique_ptr<UObject> UClass::CreateInstance() const {
    return mFactory != nullptr ? mFactory() : nullptr;
}

void UClass::Seal() {
    if (mClassSealed) {
        return;
    }

    UStruct::Seal();

    if (mParent != nullptr) {
        mAllProperties = mParent->GetAllProperties();
    }

    for (const FProperty* Property : GetProperties()) {
        if (std::any_of(mAllProperties.begin(), mAllProperties.end(), [Property](const FProperty* Existing) {
            return Existing->GetName() == Property->GetName();
        })) {
            ErrorHandler::Fatal("Reflection", "Duplicate inherited property: " + Property->GetName());
        }

        mAllProperties.push_back(Property);
    }

    mClassSealed = true;
}

const TArray<const FProperty*>& UClass::GetAllProperties() const {
    return mAllProperties;
}

const FProperty* UClass::FindClassProperty(const FString& Name) const {
    const auto Position{std::find_if(mAllProperties.begin(), mAllProperties.end(), [&Name](const FProperty* Property) {
        return Property->GetName() == Name;
    })};

    return Position != mAllProperties.end() ? *Position : nullptr;
}

void UClass::SerializeClassProperties(void* Container, FArchive& Archive) const {
    for (const FProperty* Property : mAllProperties) {
        Property->SerializeValue(Container, Archive);
    }
}
