#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

UEnum::UEnum(FString Name, std::size_t Size)
	: UStruct{std::move(Name), Size} {
}

UEnum::~UEnum() = default;

void UEnum::AddValues(std::initializer_list<FEnumValue> Values) {
    if (!mValues.empty()) {
        ErrorHandler::Fatal("Reflection", "Enum values are already registered: " + GetName());
    }

    for (const FEnumValue& Value : Values) {
        if (std::any_of(mValues.begin(), mValues.end(), [&Value](const FEnumValue& Existing) {
            return Existing.mName == Value.mName || Existing.mValue == Value.mValue;
        })) {
            ErrorHandler::Fatal("Reflection", "Duplicate enum value: " + Value.mName);
        }

        mValues.push_back(Value);
    }
}

const TArray<FEnumValue>& UEnum::GetValues() const {
    return mValues;
}
