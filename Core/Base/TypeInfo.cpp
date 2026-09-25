#include "pch.h"
#include "TypeInfo.h"

[[nodiscard]] bool FTypeInfo::IsA(const FTypeInfo* Type) const noexcept {
    for (const FTypeInfo* Current{this}; Current != nullptr; Current = Current->mParent) {
        if (Current == Type) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool FTypeInfo::IsExactlyA(const FTypeInfo* Type) const noexcept {
    return this == Type;
}
