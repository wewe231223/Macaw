#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

UStruct::UStruct(FString Name, std::size_t Size)
	: mName{std::move(Name)},
	  mSize{Size} {
}

UStruct::~UStruct() = default;

const FString& UStruct::GetName() const {
    return mName;
}

std::size_t UStruct::GetSize() const {
    return mSize;
}

void UStruct::AddProperty(FProperty Property) {
    if (mSealed || std::any_of(mDeclaredProperties.begin(), mDeclaredProperties.end(), [&Property](const FProperty& Existing) {
        return Existing.GetName() == Property.GetName();
    })) {
        ErrorHandler::Fatal("Reflection", "Duplicate or sealed property: " + Property.GetName());
    }

    mDeclaredProperties.push_back(std::move(Property));
}

void UStruct::Seal() {
    if (mSealed) {
        return;
    }

    for (const FProperty& Property : mDeclaredProperties) {
        mProperties.push_back(&Property);
    }

    mSealed = true;
}

const TArray<const FProperty*>& UStruct::GetProperties() const {
    return mProperties;
}

const FProperty* UStruct::FindProperty(const FString& Name) const {
    const auto Position{std::find_if(mProperties.begin(), mProperties.end(), [&Name](const FProperty* Property) {
        return Property->GetName() == Name;
    })};

    return Position != mProperties.end() ? *Position : nullptr;
}

bool UStruct::ContainsProperty(const FProperty& Property) const {
    return std::find(mProperties.begin(), mProperties.end(), &Property) != mProperties.end();
}

void UStruct::SerializeProperties(void* Container, FArchive& Archive) const {
    for (const FProperty* Property : mProperties) {
        Property->SerializeValue(Container, Archive);
    }
}
