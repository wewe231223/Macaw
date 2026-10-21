#include "CoreUObject/Reflection/FPropertyValue.h"

void* GetPropertyData(FPropertyValue& Value) {
    return std::visit([](auto& Typed) -> void* {
        return &Typed;
    }, Value);
}
