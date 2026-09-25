#pragma once

#include "Core/Common.h"

using FViewportId = Uint32;

enum class EProjectionType : Uint8 {
    Perspective,
    Orthographic
};

enum class EOrthographicView : Uint8 {
    Front,
    Back,
    Left,
    Right,
    Top,
    Bottom
};
