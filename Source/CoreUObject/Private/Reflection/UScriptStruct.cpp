#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <utility>

UScriptStruct::UScriptStruct(FString Name, std::size_t Size, const UScriptStruct* Parent)
	: UStruct{std::move(Name), Size},
	  mParent{Parent} {
}

UScriptStruct::~UScriptStruct() = default;

const UScriptStruct* UScriptStruct::GetSuperStruct() const {
    return mParent;
}

bool UScriptStruct::IsChildOf(const UScriptStruct* Parent) const {
    for (const UScriptStruct* Current{this}; Current != nullptr; Current = Current->mParent) {
        if (Current == Parent) {
            return true;
        }
    }

    return false;
}
