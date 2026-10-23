#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

enum class EWorldType {
    Editor,
    Game,
    Preview,
    PIE
};

ReflectEnum(EWorldType, ReflectEnumValue(EWorldType, Editor), ReflectEnumValue(EWorldType, Game), ReflectEnumValue(EWorldType, Preview), ReflectEnumValue(EWorldType, PIE));
