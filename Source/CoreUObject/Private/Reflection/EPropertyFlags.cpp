#include "CoreUObject/Reflection/EPropertyFlags.h"

EPropertyFlags operator|(EPropertyFlags Left, EPropertyFlags Right) {
    return static_cast<EPropertyFlags>(static_cast<Uint32>(Left) | static_cast<Uint32>(Right));
}
