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
        if (Path.find('\0') != Core::FString::npos) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path).lexically_normal()};

        if (!Path.empty() && (AssetPath.has_root_path() || AssetPath.begin() == AssetPath.end() || *AssetPath.begin() == L".." || !AssetPath.has_filename() || AssetPath == L".")) {
            return false;
        }

        const Core::FString TexturePath{Core::FilePath::ToGenericUtf8(AssetPath)};
        Asset::UTexture2D* Texture{LoadTexture(TexturePath)};

        if (!TexturePath.empty() && Texture == nullptr && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr) {
            return false;
        }

        mTexturePath = TexturePath;
        mTexture = Texture;
        SynchronizeDecal();

        return true;
    }

    void Engine::UDecalComponent::ResolveTexture() {
        mTexture = LoadTexture(mTexturePath);
        SynchronizeDecal();
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
        Engine::UActorComponent::UnregisterComponent();
    }

    void Engine::UDecalComponent::SynchronizeDecal() {
        Engine::FSceneInterface* Scene{GetWorld() != nullptr ? GetWorld()->GetScene() : nullptr};

        if (Scene == nullptr) {
            return;
        }

        const Core::FMatrix DecalToWorld{GetDecalToWorld()};
        Core::FMatrix WorldToDecal{};

        if (mTexture == nullptr || mOpacity <= 0.0f || !IsOwnerVisible() || !DecalToWorld.TryInverse(WorldToDecal)) {
            Scene->RemoveDecal(GetGuid());

            return;
        }

        Scene->AddDecal({GetGuid(), DecalToWorld, WorldToDecal, mTexture, mOpacity, mSortOrder});
    }

    void Engine::UDecalComponent::OnRegister() {
        ResolveTexture();
    }

    Asset::UTexture2D* Engine::UDecalComponent::LoadTexture(const Core::FString& Path) const {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || Path.empty()) {
            return nullptr;
        }

        Core::FString Error{};
        Asset::UTexture2D* Texture{Assets->LoadTexture(Core::FilePath::FromUtf8(Path), Error)};

        if (Texture == nullptr) {
            Core::ErrorHandler::Report("Decal", "Cannot load texture '" + Path + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);
        }

        return Texture;
    }

}
