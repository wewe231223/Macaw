#pragma once

#include "Core/Guid.h"
#include "Core/Math.h"

#include <variant>

using FPropertyValue = std::variant<bool, Int32, Uint32, Uint64, float, FString, FGuid, FVector2, FVector, FVector4, FQuat, FRotator, FMatrix, FTransform, FBox, FRay, const void*>;

void* GetPropertyData(FPropertyValue& Value);
