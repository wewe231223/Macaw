#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

enum class EEditorViewportType : Uint8 {
    Perspective,
    Top,
    Bottom,
    Front,
    Back,
    Left,
    Right
};

ReflectEnum(EEditorViewportType, Perspective, Top, Bottom, Front, Back, Left, Right);
