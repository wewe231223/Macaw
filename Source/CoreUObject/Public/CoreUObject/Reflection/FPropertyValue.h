#pragma once

#include "Core/Guid.h"
#include "Core/Math/FTransform.h"

#include <variant>

using FPropertyValue = std::variant<bool, Int32, Uint32, float, FString, FGuid, FVector, FRotator, FTransform>;

void* GetPropertyData(FPropertyValue& Value);
