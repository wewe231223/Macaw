#include "CoreUObject/Reflection/EClassFlags.h"

EClassFlags operator|(EClassFlags Left, EClassFlags Right) {
    return static_cast<EClassFlags>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}
