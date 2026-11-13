/**
 * @file UDecalComponent.cpp
 * @brief 박스 영역에 색상·노멀 텍스처를 투영하고 데칼 프록시를 장면과 동기화합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UDecalComponent.h"

#include "Asset/FAssetManager.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "Engine/FDecalSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/UWorld.h"

#include <cmath>

namespace Engine {

    Engine::UDecalComponent::~UDecalComponent() {
        UnregisterComponent();
    }

    const Core::FString& Engine::UDecalComponent::GetTexturePath() const {
        return mTexturePath;
    }

    bool Engine::UDecalComponent::SetTexturePath(const Core::FString& Path) {
        return SetDecalTexturePath(Path, false);
    }

    const Core::FString& Engine::UDecalComponent::GetNormalMapPath() const {
        return mNormalMapPath;
    }

    bool Engine::UDecalComponent::SetNormalMapPath(const Core::FString& Path) {
        return SetDecalTexturePath(Path, true);
    }

    void Engine::UDecalComponent::ResolveTexture() {
        mTexture = LoadTexture(mTexturePath, false);
        mNormalMap = LoadTexture(mNormalMapPath, true);
        SynchronizeDecal();
    }

    float Engine::UDecalComponent::GetNormalStrength() const {
        return mNormalStrength;
    }

    bool Engine::UDecalComponent::SetNormalStrength(float Strength) {
        if (!std::isfinite(Strength) || Strength < 0.0f || Strength > 8.0f) {
            return false;
        }

        mNormalStrength = Strength;
        SynchronizeDecal();

        return true;
    }

    const Core::FVector& Engine::UDecalComponent::GetExtent() const {
        return mExtent;
    }

    bool Engine::UDecalComponent::SetExtent(const Core::FVector& Extent) {
        if (!Extent.IsFinite() || Extent.x <= 0.0f || Extent.y <= 0.0f || Extent.z <= 0.0f) {
            return false;
        }

        mExtent = Extent;
        SynchronizeDecal();

        return true;
    }

    Core::FMatrix Engine::UDecalComponent::GetDecalToWorld() const {
        return Core::FMatrix::Scale(mExtent) * GetWorldMatrix();
    }

    float Engine::UDecalComponent::GetOpacity() const {
        return mOpacity;
    }

    bool Engine::UDecalComponent::SetOpacity(float Opacity) {
        if (!std::isfinite(Opacity) || Opacity < 0.0f || Opacity > 1.0f) {
            return false;
        }

        mOpacity = Opacity;
        SynchronizeDecal();

        return true;
    }

    Core::Int32 Engine::UDecalComponent::GetSortOrder() const {
        return mSortOrder;
    }

    void Engine::UDecalComponent::SetSortOrder(Core::Int32 SortOrder) {
        mSortOrder = SortOrder;
        SynchronizeDecal();
    }

    bool Engine::UDecalComponent::SetTransform(const Core::FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        SynchronizeDecal();

        return true;
    }

    void Engine::UDecalComponent::OnOwnerVisibilityChanged() {
        SynchronizeDecal();
    }

    void Engine::UDecalComponent::UnregisterComponent() {
        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->RemoveDecal(GetGuid());
        }

        mTexture = nullptr;
        mNormalMap = nullptr;
        Engine::UActorComponent::UnregisterComponent();
    }

    void Engine::UDecalComponent::SynchronizeDecal() {
        Engine::FSceneInterface* Scene{GetWorld() != nullptr ? GetWorld()->GetScene() : nullptr};

        if (Scene == nullptr) {
            return;
        }

        const Core::FMatrix DecalToWorld{GetDecalToWorld()};
        Core::FMatrix WorldToDecal{};

        const bool HasNormal{mNormalMap != nullptr && mNormalStrength > 0.0f};

        if ((mTexture == nullptr && !HasNormal) || mOpacity <= 0.0f || !IsOwnerVisible() || !DecalToWorld.TryInverse(WorldToDecal)) {
            Scene->RemoveDecal(GetGuid());

            return;
        }

        Scene->AddDecal({GetGuid(), DecalToWorld, WorldToDecal, mTexture, mOpacity, mSortOrder, mNormalMap, mNormalStrength});
    }

    void Engine::UDecalComponent::OnRegister() {
        ResolveTexture();
    }

    bool Engine::UDecalComponent::SetDecalTexturePath(const Core::FString& Path, bool NormalMap) {
        if (Path.find('\0') != Core::FString::npos) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path).lexically_normal()};

        if (!Path.empty() && (AssetPath.has_root_path() || AssetPath.begin() == AssetPath.end() || *AssetPath.begin() == L".." || !AssetPath.has_filename() || AssetPath == L".")) {
            return false;
        }

        const Core::FString TexturePath{Core::FilePath::ToGenericUtf8(AssetPath)};
        Asset::UTexture2D* Texture{LoadTexture(TexturePath, NormalMap)};

        if (!TexturePath.empty() && Texture == nullptr && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr) {
            return false;
        }

        (NormalMap ? mNormalMapPath : mTexturePath) = TexturePath;
        (NormalMap ? mNormalMap : mTexture) = Texture;
        SynchronizeDecal();

        return true;
    }

    Asset::UTexture2D* Engine::UDecalComponent::LoadTexture(const Core::FString& Path, bool NormalMap) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};
        Asset::UTexture2D* Texture{NormalMap ? Assets->LoadNormalMap(AssetPath, Error) : Assets->LoadTexture(AssetPath, Error)};

        if (Texture == nullptr) {
            Core::ErrorHandler::Report("Decal", "Cannot load texture '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Texture;
    }

}
