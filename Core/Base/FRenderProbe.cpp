#include "pch.h"
#include "FRenderProbe.h"

Uint32 operator|(ERenderObjectFlags Left, ERenderObjectFlags Right) {
    return static_cast<Uint32>(Left) | static_cast<Uint32>(Right);
}
