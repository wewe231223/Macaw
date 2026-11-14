#include "Engine/Cascade/UParticleModuleRequired.h"
#include "Asset/FAssetManager.h"
#include "Asset/UMaterial.h"
#include "Core/FilePath.h"

#include <cmath>
#include <filesystem>

using namespace Engine;
using namespace Core;

UParticleModuleRequired::UParticleModuleRequired() {
    SetName("Required");
}

UParticleModuleRequired::~UParticleModuleRequired() = default;

Asset::UMaterial* UParticleModuleRequired::GetMaterial() const {
    return mMaterial.Get();
}

bool UParticleModuleRequired::SetMaterial(Asset::UMaterial* Material, const Asset::FAssetManager& Assets) {
    if (Material == nullptr) {
        return SetMaterialPath({});
    }

    const FString Path{Assets.GetMaterialPath(Material)};

    if (Path.empty() || !SetMaterialPath(Path)) {
        return false;
    }

    mMaterial = Material;

    return true;
}

bool UParticleModuleRequired::ResolveMaterial(Asset::FAssetManager& Assets, FString& Error) {
    Error.clear();

    if (mMaterialPath.empty()) {
        mMaterial.Reset();

        return true;
    }

    Asset::UMaterial* Material{Assets.LoadMaterial(FilePath::FromUtf8(mMaterialPath), Error)};

    if (Material == nullptr) {
        return false;
    }

    mMaterial = Material;

    return true;
}

const FString& UParticleModuleRequired::GetMaterialPath() const {
    return mMaterialPath;
}

bool UParticleModuleRequired::SetMaterialPath(const FString& Path) {
    if (Path.find('\0') != FString::npos) {
        return false;
    }

    const std::filesystem::path AssetPath{FilePath::FromUtf8(Path)};

    if (!Path.empty() && (AssetPath.has_root_path() || FilePath::ToLowerExtension(AssetPath) != L".masset")) {
        return false;
    }

    for (const auto& Part : AssetPath) {
        if (Part == L"..") {
            return false;
        }
    }

    mMaterialPath = Path.empty() ? FString{} : FilePath::ToGenericUtf8(AssetPath.lexically_normal());
    mMaterial.Reset();

    return true;
}

const FVector& UParticleModuleRequired::GetEmitterOrigin() const {
    return mEmitterOrigin;
}

bool UParticleModuleRequired::SetEmitterOrigin(const FVector& Origin) {
    if (!Origin.IsFinite()) {
        return false;
    }

    mEmitterOrigin = Origin;

    return true;
}

const FRotator& UParticleModuleRequired::GetEmitterRotation() const {
    return mEmitterRotation;
}

bool UParticleModuleRequired::SetEmitterRotation(const FRotator& Rotation) {
    if (!Rotation.IsFinite()) {
        return false;
    }

    mEmitterRotation = Rotation;

    return true;
}

bool UParticleModuleRequired::GetUseLocalSpace() const {
    return mUseLocalSpace;
}

void UParticleModuleRequired::SetUseLocalSpace(bool UseLocalSpace) {
    mUseLocalSpace = UseLocalSpace;
}

float UParticleModuleRequired::GetEmitterDuration() const {
    return mEmitterDuration;
}

bool UParticleModuleRequired::SetEmitterDuration(float Duration) {
    if (!std::isfinite(Duration) || Duration <= 0.0f) {
        return false;
    }

    mEmitterDuration = Duration;

    return true;
}

Uint32 UParticleModuleRequired::GetEmitterLoops() const {
    return mEmitterLoops;
}

void UParticleModuleRequired::SetEmitterLoops(Uint32 Loops) {
    mEmitterLoops = Loops;
}
