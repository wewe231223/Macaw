#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <utility>

UScriptStruct::UScriptStruct(FString Name, std::size_t Size)
	: UStruct{std::move(Name), Size} {
}

UScriptStruct::~UScriptStruct() = default;
