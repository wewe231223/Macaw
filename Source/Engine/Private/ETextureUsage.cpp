#include "Engine/ETextureUsage.h"

ETextureUsage operator|(ETextureUsage Left, ETextureUsage Right) {
    return static_cast<ETextureUsage>(static_cast<Uint8>(Left) | static_cast<Uint8>(Right));
}

ETextureUsage operator&(ETextureUsage Left, ETextureUsage Right) {
    return static_cast<ETextureUsage>(static_cast<Uint8>(Left) & static_cast<Uint8>(Right));
}
