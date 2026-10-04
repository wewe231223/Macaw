#include "pch.h"
#include "Core/Channel/FMessageTypeInfo.h"

bool FMessageTypeInfo::IsExactlyA(const FMessageTypeInfo* Type) const noexcept {
    return this == Type;
}
