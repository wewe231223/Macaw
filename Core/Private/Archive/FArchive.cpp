#include "pch.h"
#include "Core/Archive/FArchive.h"

FArchive::FArchive(EArchiveMode InMode)
	: mMode{InMode},
	  mAssetResolver{nullptr} {
}

bool FArchive::HasError() const {
    return mError;
}

bool FArchive::SupportsNamedFields() const {
    return false;
}

bool FArchive::HasField(std::string_view Name) const {
    return false;
}

void FArchive::SetError() {
    mError = true;
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
        if (HasError() || Str.size() >= NameSize) {
            SetError();
            return;
        }

        Value = FName{Str};
    }
}

void FArchive::SerializeObjectGuid(std::string_view Name, FGuid& Value) {
    Serialize(Name, Value);

    if (IsLoading() && !HasError()) {
        RemapObjectGuid(Value);
    }
}

bool FArchive::RemapObjectReference(FGuid& Guid) {
    return !Guid.IsValid() || RemapObjectGuid(Guid);
}

void FArchive::SetObjectGuidRemapping(const TMap<FGuid, FGuid>* Remapping) {
    mObjectGuidRemapping = Remapping;
}

const TMap<FGuid, FGuid>* FArchive::GetObjectGuidRemapping() const {
    return mObjectGuidRemapping;
}

bool FArchive::RemapObjectGuid(FGuid& Guid) {
    if (mObjectGuidRemapping == nullptr) {
        return true;
    }

    const auto Found{mObjectGuidRemapping->find(Guid)};

    if (Found == mObjectGuidRemapping->end() || !Found->second.IsValid()) {
        SetError();
        Guid = {};
        return false;
    }

    Guid = Found->second;

    return true;
}
