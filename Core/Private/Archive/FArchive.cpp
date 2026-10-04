#include "pch.h"
#include "Core/Archive/FArchive.h"

FArchive::FArchive(EArchiveMode InMode)
	: mMode(InMode),
	  mAssetResolver(nullptr) {
}

void FArchive::SetAssetResolver(const IAssetResolver* InAssetResolver) {
    mAssetResolver = InAssetResolver;
}

const IAssetResolver* FArchive::GetAssetResolver() {
    return mAssetResolver;
}

bool FArchive::IsLoading() const {
    return mMode == EArchiveMode::Loading;
}

bool FArchive::IsSaving() const {
    return mMode == EArchiveMode::Saving;
}

bool FArchive::IsHashing() const {
    return mMode == EArchiveMode::Hashing;
}

bool FArchive::IsCounting() const {
    return mMode == EArchiveMode::Counting;
}

void FArchive::Serialize(std::string_view Name, FName& Value) {
    FString Str{Value.ToString()};

    Serialize(Name, Str);

    if (IsLoading()) {
        Value = FName{Str};
    }
}
