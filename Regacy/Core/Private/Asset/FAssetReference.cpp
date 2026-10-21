#include "pch.h"
#include "Core/Asset/FAssetReference.h"
#include "Core/Asset/IAssetResolver.h"

FAssetReference::FAssetReference(const FGuid& Guid, const FAssetPath& Path)
	: mGuid{Guid},
	  mPath{Path} {
}

FAssetHandle FAssetReference::GetHandle() const {
    return mHandle;
}

const FGuid& FAssetReference::GetGuid() const {
    return mGuid;
}

const FAssetPath& FAssetReference::GetPath() const {
    return mPath;
}

bool FAssetReference::IsEmpty() const {
    return !mHandle && !mGuid.IsValid() && !mPath.IsValid();
}

void FAssetReference::Assign(FAssetHandle Handle, const IAssetResolver* Resolver) {
    if (!Handle || Handle != mHandle) {
        mGuid = {};
        mPath = {};
    }

    mHandle = Handle;
    RefreshIdentity(Resolver);
}

FAssetHandle FAssetReference::Resolve(const IAssetResolver* Resolver) {
    if (Resolver == nullptr) {
        return mHandle;
    }

    if (mGuid.IsValid() || mPath.IsValid()) {
        mHandle = mGuid.IsValid() ? Resolver->FindAsset(mGuid) : FAssetHandle{};

        if (!mHandle && mPath.IsValid()) {
            mHandle = Resolver->FindAsset(mPath);
        }
    }

    RefreshIdentity(Resolver);
    return mHandle;
}

void FAssetReference::RefreshIdentity(const IAssetResolver* Resolver) {
    if (Resolver == nullptr || !mHandle) {
        return;
    }

    if (const FGuid* Guid{Resolver->GetAssetGuid(mHandle)}; Guid != nullptr && Guid->IsValid()) {
        mGuid = *Guid;
    }

    if (const FAssetPath* Path{Resolver->GetAssetPath(mHandle)}; Path != nullptr && Path->IsValid()) {
        mPath = *Path;
    }
}
