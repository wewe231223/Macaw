#include "pch.h"
#include "TGraphicsRootConstants.h"

EGraphicsShaderStage operator|(EGraphicsShaderStage Left, EGraphicsShaderStage Right) {
    return static_cast<EGraphicsShaderStage>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}

EGraphicsShaderStage operator&(EGraphicsShaderStage Left, EGraphicsShaderStage Right) {
    return static_cast<EGraphicsShaderStage>(static_cast<Uint32>(Left) & static_cast<Uint32>(Right));
}

EGraphicsShaderStage& operator|=(EGraphicsShaderStage& Left, EGraphicsShaderStage Right) {
    Left = Left | Right;
    return Left;
}

bool HasGraphicsShaderStage(EGraphicsShaderStage Value, EGraphicsShaderStage Stage) {
    return static_cast<Uint32>(Value & Stage) != 0;
}
