#include "pch.h"
#include "Asset/UFont.h"

const FTypeInfo* UFont::StaticTypeInfo() noexcept {
    static const FTypeInfo Information{"UFont", UAsset::StaticTypeInfo(), nullptr};
    return &Information;
}

const FTypeInfo* UFont::GetTypeInfo() const noexcept {
    return StaticTypeInfo();
}
