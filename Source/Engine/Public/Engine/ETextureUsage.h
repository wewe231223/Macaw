#pragma once

#include "Core/Types.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

enum class ETextureUsage : Uint8 {
    None = 0,
    ShaderResource = 1,
    RenderTarget = 2,
    DepthStencil = 4
};

ReflectEnum(ETextureUsage, None, ShaderResource, RenderTarget, DepthStencil);

ETextureUsage operator|(ETextureUsage Left, ETextureUsage Right);
ETextureUsage operator&(ETextureUsage Left, ETextureUsage Right);
