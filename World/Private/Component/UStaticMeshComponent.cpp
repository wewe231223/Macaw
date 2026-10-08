#include "pch.h"
#include "World/Component/UStaticMeshComponent.h"
#include "RenderCore/FRenderData.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "Core/Archive/FArchive.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UMaterial.h"
#include "Asset/Pipeline/UPipeline.h"

namespace {
    constexpr char BasePipelinePath[]{"/Game/Pipeline/Base"};
    constexpr char TextureBasePipelinePath[]{"/Game/Pipeline/TexturedBase"};
}

FAssetHandle UStaticMeshComponent::GetMaterialHandle() const {
    return mMaterialHandle;
}

FAssetHandle UStaticMeshComponent::GetPipelineHandle() const {
    return mPipelineHandle;
}

void UStaticMeshComponent::SetMeshHandle(FAssetHandle InHandle) {
    const FAssetHandle PreviousHandle{GetMeshHandle()};

    UMeshComponent::SetMeshHandle(InHandle);

    if (PreviousHandle != GetMeshHandle()) {
        OnRenderStateChanged();
    }
}

void UStaticMeshComponent::SetMaterialHandle(FAssetHandle InHandle) {
    const FAssetHandle PreviousMaterialHandle{mMaterialHandle};
    const FAssetHandle PreviousPipelineHandle{mPipelineHandle};

    mMaterialHandle = InHandle;

    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    mMaterialAssetPath = Registry != nullptr && Registry->GetAssetPath(mMaterialHandle) != nullptr ? *Registry->GetAssetPath(mMaterialHandle) : FAssetPath{};
    mMaterialAssetGuid = Registry != nullptr && Registry->GetAssetGuid(mMaterialHandle) != nullptr ? *Registry->GetAssetGuid(mMaterialHandle) : FGuid{};

    EnsureDefaultRenderAssets();

    if (PreviousMaterialHandle != mMaterialHandle || PreviousPipelineHandle != mPipelineHandle) {
        OnRenderStateChanged();
    }
}

void UStaticMeshComponent::SetPipelineHandle(FAssetHandle InHandle) {
    const FAssetHandle PreviousMaterialHandle{mMaterialHandle};
    const FAssetHandle PreviousPipelineHandle{mPipelineHandle};

    mPipelineHandle = InHandle;

    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    mPipelineAssetPath = Registry != nullptr && Registry->GetAssetPath(mPipelineHandle) != nullptr ? *Registry->GetAssetPath(mPipelineHandle) : FAssetPath{};
    mPipelineAssetGuid = Registry != nullptr && Registry->GetAssetGuid(mPipelineHandle) != nullptr ? *Registry->GetAssetGuid(mPipelineHandle) : FGuid{};

    EnsureDefaultRenderAssets();

    if (PreviousMaterialHandle != mMaterialHandle || PreviousPipelineHandle != mPipelineHandle) {
        OnRenderStateChanged();
    }
}

void UStaticMeshComponent::OnRegister() {
    UMeshComponent::OnRegister();

    EnsureDefaultRenderAssets();

}

void UStaticMeshComponent::EnsureDefaultRenderAssets() {
    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry == nullptr) {
        return;
    }

    if (Registry->ResolveAsset<UMaterial>(mMaterialHandle) == nullptr) {
        mMaterialHandle = Registry->EnsureDefaultStaticMeshMaterial();
    }

    if (Registry->ResolveAsset<UPipeline>(mPipelineHandle) == nullptr) {
        mPipelineHandle = Registry->EnsureDefaultStaticMeshPipeline();
    }
}

bool UStaticMeshComponent::ShouldCreateRenderState() const {
    return IsVisible();
}

std::unique_ptr<FPrimitiveSceneProxy> UStaticMeshComponent::CreateSceneProxy() const {
    const AActor* Owner{GetOwner()};

    if (!IsRegistered() || !IsVisible() || Owner == nullptr) {
        return nullptr;
    }

    return std::make_unique<FStaticMeshSceneProxy>(GetHandle(), Owner->GetHandle(), GetRenderTransform(), FMeshSceneData{GetMeshHandle(), mMaterialHandle, mPipelineHandle});
}

void UStaticMeshComponent::Serialize(FArchive& Archive) {
    UMeshComponent::Serialize(Archive);

    const IAssetResolver* Registry{Archive.GetAssetResolver()};

    if (Archive.IsSaving() && Registry != nullptr) {
        if (const FAssetPath* AssetPath{Registry->GetAssetPath(mMaterialHandle)}) {
            mMaterialAssetPath = *AssetPath;
        }

        if (const FGuid* AssetGuid{Registry->GetAssetGuid(mMaterialHandle)}) {
            mMaterialAssetGuid = *AssetGuid;
        }

        if (const FAssetPath* AssetPath{Registry->GetAssetPath(mPipelineHandle)}) {
            mPipelineAssetPath = *AssetPath;
        }

        if (const FGuid* AssetGuid{Registry->GetAssetGuid(mPipelineHandle)}) {
            mPipelineAssetGuid = *AssetGuid;
        }
    }

    Archive.Serialize("MaterialAssetGuid", mMaterialAssetGuid);
    Archive.Serialize("MaterialAssetPath", mMaterialAssetPath.mPath);
    Archive.Serialize("PipelineAssetGuid", mPipelineAssetGuid);
    Archive.Serialize("PipelineAssetPath", mPipelineAssetPath.mPath);

    if (Archive.IsLoading()) {
        mMaterialHandle = Registry != nullptr ? Registry->FindAsset(mMaterialAssetGuid) : FAssetHandle{};

        if (!mMaterialHandle && Registry != nullptr) {
            mMaterialHandle = Registry->FindAsset(mMaterialAssetPath);
        }

        mPipelineHandle = Registry != nullptr ? Registry->FindAsset(mPipelineAssetGuid) : FAssetHandle{};

        if (!mPipelineHandle && Registry != nullptr) {
            mPipelineHandle = Registry->FindAsset(mPipelineAssetPath);
        }

        BuildPickingBoxFromMesh();
        OnRenderStateChanged();
    }
}
