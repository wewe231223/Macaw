#include "pch.h"
#include "FAssetPath.h"

FAssetPath::FAssetPath(const FString& InPath)
    : mPath(InPath) {
}

FAssetPath::FAssetPath(FString&& InPath)
    : mPath(std::move(InPath)) {
}

bool FAssetPath::IsValid() const {
    return !mPath.empty();
}

FAssetPath::operator bool() const {
    return IsValid();
}

std::size_t std::hash<FAssetPath>::operator()(const FAssetPath& AssetPath) const noexcept {
    return std::hash<FString>{}(AssetPath.mPath);
}
