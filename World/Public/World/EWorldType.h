#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/EWorldType.generated.h"

UENUM()
enum class EWorldType {
    Editor,
    Game,
    Preview
};
