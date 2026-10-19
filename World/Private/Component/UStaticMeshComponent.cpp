#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
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

FAssetHandle UStaticMeshComponent::GetMaterialHandle() const {
    return mMaterialReference.GetHandle();
}

FAssetHandle UStaticMeshComponent::GetPipelineHandle() const {
    return mPipelineReference.GetHandle();
}

void UStaticMeshComponent::SetMaterialHandle(FAssetHandle InHandle) {
    mMaterialReference.Assign(InHandle, FAssetRegistry::TryGet());
    OnRenderStateChanged();
}

void UStaticMeshComponent::SetPipelineHandle(FAssetHandle InHandle) {
    mPipelineReference.Assign(InHandle, FAssetRegistry::TryGet());
    OnRenderStateChanged();
}

void UStaticMeshComponent::OnRegister() {
    Super::OnRegister();

    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    mMaterialReference.Resolve(Registry);
    mPipelineReference.Resolve(Registry);
}

bool UStaticMeshComponent::ShouldCreateRenderState() const {
    return IsVisible();
}

std::unique_ptr<FPrimitiveSceneProxy> UStaticMeshComponent::CreateSceneProxy() const {
    const AActor* Owner{GetOwner()};

    if (!IsRegistered() || !IsVisible() || Owner == nullptr) {
        return nullptr;
    }

    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};
    const FAssetHandle Material{Registry != nullptr && Registry->ResolveAsset<UMaterial>(GetMaterialHandle()) == nullptr ? Registry->EnsureDefaultStaticMeshMaterial() : GetMaterialHandle()};
    const FAssetHandle Pipeline{Registry != nullptr && Registry->ResolveAsset<UPipeline>(GetPipelineHandle()) == nullptr ? Registry->EnsureDefaultStaticMeshPipeline() : GetPipelineHandle()};
    std::unique_ptr<FStaticMeshSceneProxy> Proxy{std::make_unique<FStaticMeshSceneProxy>(GetHandle(), Owner->GetHandle(), GetRenderTransform(), FMeshSceneData{GetMeshHandle(), Material, Pipeline})};

    Proxy->SetUVTransforms(GetUVTransforms());
    return Proxy;
}

UStaticMeshComponent::UStaticMeshComponent() {
    InitializeClass(StaticClass());
}

void UStaticMeshComponent::SetEditorMaterialHandle(FAssetHandle InHandle) {
    SetMaterialHandle(InHandle);

    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr) {
        return;
    }

    const UMaterial* Material{Registry->ResolveAsset<UMaterial>(GetMaterialHandle())};

    if (Material == nullptr || Registry->ResolveAsset<UPipeline>(GetPipelineHandle()) == nullptr) {
        return;
    }

    const FAssetHandle BasePipeline{Registry->FindAsset(FAssetPath{"/Game/Pipeline/Base"})};
    const FAssetHandle TexturedPipeline{Registry->FindAsset(FAssetPath{"/Game/Pipeline/TexturedBase"})};

    if (GetPipelineHandle() != BasePipeline && GetPipelineHandle() != TexturedPipeline) {
        return;
    }

    bool HasTexture{};

    for (Uint32 GroupIndex{}; GroupIndex < Material->GetGPUDataCount() && !HasTexture; ++GroupIndex) {
        const FMaterialChunkSignature Signature{Material->BuildChunkSignature(GroupIndex)};

        for (Uint8 TextureFieldIndex{}; TextureFieldIndex < Signature.mTextureFieldCount; ++TextureFieldIndex) {
            if (Signature.GetTextureHandle(TextureFieldIndex)) {
                HasTexture = true;
                break;
            }
        }
    }

    const FAssetHandle DesiredPipelineHandle{HasTexture ? TexturedPipeline : BasePipeline};

    if (DesiredPipelineHandle != GetPipelineHandle() && Registry->ResolveAsset<UPipeline>(DesiredPipelineHandle) != nullptr) {
        SetPipelineHandle(DesiredPipelineHandle);
    }
}

void UStaticMeshComponent::SerializeMaterialHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UStaticMeshComponent& Object{*static_cast<UStaticMeshComponent*>(Container)};

    SerializeAssetReference(Archive, "MaterialAssetGuid", "MaterialAssetPath", Object.mMaterialReference);
}

void UStaticMeshComponent::SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UStaticMeshComponent& Object{*static_cast<UStaticMeshComponent*>(Container)};

    SerializeAssetReference(Archive, "PipelineAssetGuid", "PipelineAssetPath", Object.mPipelineReference);
}
