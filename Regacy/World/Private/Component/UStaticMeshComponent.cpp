#include "pch.h"
#include "World/Component/UStaticMeshComponent.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "RenderCore/FStaticMeshSceneProxy.h"
#include "World/AActor.h"
#include <algorithm>

void FMeshMaterialOverride::Serialize(FArchive& Archive) {
    Archive.Serialize("Slot", mSlot);
    SerializeAssetReference(Archive, "MaterialGuid", "MaterialPath", mMaterial);
}

UStaticMeshComponent::UStaticMeshComponent() {
    InitializeClass(StaticClass());
}

FAssetHandle UStaticMeshComponent::GetMaterial(Uint32 SlotIndex) const {
    const UMesh* Mesh{ResolveMesh()};

    if (Mesh == nullptr || SlotIndex >= Mesh->GetMaterialSlots().size()) {
        return {};
    }

    const FAssetHandle Override{GetMaterialOverride(Mesh->GetMaterialSlotName(SlotIndex))};
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};
    const FAssetHandle Material{Registry != nullptr && Registry->ResolveAsset<UMaterial>(Override) != nullptr ? Override : Mesh->GetDefaultMaterial(SlotIndex)};

    return Registry != nullptr && Registry->ResolveAsset<UMaterial>(Material) == nullptr ? Registry->EnsureDefaultStaticMeshMaterial() : Material;
}

FAssetHandle UStaticMeshComponent::GetMaterialOverride(FName Slot) const {
    const auto Position{std::ranges::find(mMaterialOverrides, Slot, &FMeshMaterialOverride::mSlot)};

    return Position != mMaterialOverrides.end() ? Position->mMaterial.GetHandle() : FAssetHandle{};
}

bool UStaticMeshComponent::SetMaterialOverride(FName Slot, FAssetHandle Material) {
    const UMesh* Mesh{ResolveMesh()};

    if (Mesh == nullptr || !Mesh->FindMaterialSlot(Slot).has_value()) {
        return false;
    }

    const auto Position{std::ranges::find(mMaterialOverrides, Slot, &FMeshMaterialOverride::mSlot)};

    if (!Material) {
        if (Position != mMaterialOverrides.end()) {
            mMaterialOverrides.erase(Position);
            OnRenderStateChanged();
        }

        return true;
    }

    FMeshMaterialOverride Override{Slot, {}};

    Override.mMaterial.Assign(Material, FAssetRegistry::TryGet());

    if (Position == mMaterialOverrides.end()) {
        mMaterialOverrides.push_back(std::move(Override));
    } else if (Position->mMaterial.GetHandle() == Material) {
        return true;
    } else {
        *Position = std::move(Override);
    }

    OnRenderStateChanged();
    return true;
}

void UStaticMeshComponent::ClearMaterialOverrides() {
    if (!mMaterialOverrides.empty()) {
        mMaterialOverrides.clear();
        OnRenderStateChanged();
    }
}

void UStaticMeshComponent::OnRegister() {
    Super::OnRegister();

    for (FMeshMaterialOverride& Override : mMaterialOverrides) {
        Override.mMaterial.Resolve(FAssetRegistry::TryGet());
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

    FMeshSceneData Data{GetMeshHandle(), {}};

    for (const FMeshMaterialOverride& Override : mMaterialOverrides) {
        Data.mMaterialOverrides.push_back(FMeshMaterialBinding{Override.mSlot, Override.mMaterial.GetHandle()});
    }

    std::unique_ptr<FStaticMeshSceneProxy> Proxy{std::make_unique<FStaticMeshSceneProxy>(GetHandle(), Owner->GetHandle(), GetRenderTransform(), Data)};

    Proxy->SetUVTransforms(GetUVTransforms());
    return Proxy;
}

void UStaticMeshComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading() && Archive.SupportsNamedFields() && !Archive.HasField("MaterialOverrides") && (Archive.HasField("MaterialAssetGuid") || Archive.HasField("MaterialAssetPath"))) {
        mMaterialOverrides.clear();
        FGuid MaterialGuid{};
        FAssetPath MaterialPath{};
        FAssetReference Pipeline{};

        Archive.Serialize("MaterialAssetGuid", MaterialGuid);
        Archive.Serialize("MaterialAssetPath", MaterialPath.mPath);
        SerializeAssetReference(Archive, "PipelineAssetGuid", "PipelineAssetPath", Pipeline);
        const FAssetReference Library{MaterialGuid, MaterialPath};
        FAssetRegistry* Registry{FAssetRegistry::TryGet()};
        const UMesh* Mesh{ResolveMesh()};

        if (!Library.IsEmpty() && Registry != nullptr && Mesh != nullptr) {
            for (const FMeshMaterialSlot& Slot : Mesh->GetMaterialSlots()) {
                mMaterialOverrides.push_back(FMeshMaterialOverride{Slot.mName, Registry->ImportLegacyMaterial(Library, Slot.mName, Pipeline)});
            }
        }
    }

    if (Archive.IsLoading()) {
        for (FMeshMaterialOverride& Override : mMaterialOverrides) {
            Override.mMaterial.Resolve(Archive.GetAssetResolver());
        }

        OnRenderStateChanged();
    }
}
