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

    Super::SetMeshHandle(InHandle);

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
    Super::OnRegister();

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

    std::unique_ptr<FStaticMeshSceneProxy> Proxy{std::make_unique<FStaticMeshSceneProxy>(GetHandle(), Owner->GetHandle(), GetRenderTransform(), FMeshSceneData{GetMeshHandle(), mMaterialHandle, mPipelineHandle})};

    Proxy->SetUVTransforms(GetUVTransforms());
    return Proxy;
}

void UStaticMeshComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        BuildPickingBoxFromMesh();
        OnRenderStateChanged();
    }
}

UStaticMeshComponent::UStaticMeshComponent() {
    InitializeClass(StaticClass());
}

void UStaticMeshComponent::SetEditorMaterialHandle(FAssetHandle InHandle) {
    SetMaterialHandle(InHandle);

    const UWorld* World{GetOwner() != nullptr ? GetOwner()->GetWorld() : nullptr};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry == nullptr) {
        return;
    }

    const UMaterial* Material{Registry->ResolveAsset<UMaterial>(GetMaterialHandle())};
    bool HasTexture{};

    if (Material != nullptr) {
        for (Uint32 GroupIndex{}; GroupIndex < Material->GetGPUDataCount() && !HasTexture; ++GroupIndex) {
            const FMaterialChunkSignature Signature{Material->BuildChunkSignature(GroupIndex)};

            for (Uint8 TextureFieldIndex{}; TextureFieldIndex < Signature.mTextureFieldCount; ++TextureFieldIndex) {
                if (Signature.GetTextureHandle(TextureFieldIndex)) {
                    HasTexture = true;
                    break;
                }
            }
        }
    }

    const FAssetHandle DesiredPipelineHandle{Registry->FindAsset(FAssetPath{HasTexture ? "/Game/Pipeline/TexturedBase" : "/Game/Pipeline/Base"})};

    if (DesiredPipelineHandle != GetPipelineHandle() && Registry->ResolveAsset<UPipeline>(DesiredPipelineHandle) != nullptr) {
        SetPipelineHandle(DesiredPipelineHandle);
    }
}
