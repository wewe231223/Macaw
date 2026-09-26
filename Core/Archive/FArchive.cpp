#include "pch.h"
#include "FArchive.h"

FArchive::FArchive(EArchiveMode InMode)
    : Mode(InMode),
      mAssetRegistry(nullptr) {
}

void FArchive::SetAssetRegistry(const IAssetRegistry* InputAssetRegistry) {
    mAssetRegistry = InputAssetRegistry;
}

const IAssetRegistry* FArchive::GetAssetRegistry() {
    return mAssetRegistry;
}

bool FArchive::IsLoading() const {
    return Mode == EArchiveMode::Loading;
}

bool FArchive::IsSaving() const {
    return Mode == EArchiveMode::Saving;
}

bool FArchive::IsHashing() const {
    return Mode == EArchiveMode::Hashing;
}

bool FArchive::IsCounting() const {
    return Mode == EArchiveMode::Counting;
}

void FArchive::Serialize(std::string_view Name, FName& Value) {
    FString Str{Value.ToString()};
    Serialize(Name, Str);

    if (IsLoading()) {
        Value = FName{Str};
    }
}
