#pragma once

#include "World/Component/UMeshComponent.h"
#include "Asset/UMaterial.h"

struct FMeshMaterialOverride {
public:
    void Serialize(FArchive& Archive);

public:
    FName mSlot{};
    FAssetReference mMaterial{};
};

class UStaticMeshComponent : public UMeshComponent {
public:
    ReflectTypes(UStaticMeshComponent, UMeshComponent);

public:
    UStaticMeshComponent();
    ~UStaticMeshComponent() override = default;

public:
    ReflectClass(UStaticMeshComponent);

    FAssetHandle GetMaterial(Uint32 SlotIndex) const;
    FAssetHandle GetMaterialOverride(FName Slot) const;
    bool SetMaterialOverride(FName Slot, FAssetHandle Material);
    void ClearMaterialOverrides();
    void OnRegister() override;
    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;

private:
    void Serialize(FArchive& Archive) override;

private:
    ReflectProperty(mMaterialOverrides, "MaterialOverrides", EPropertyFlags::Serialize, ReflectSerializer(mMaterialOverrides));

    TArray<FMeshMaterialOverride> mMaterialOverrides{};

    ReflectMembers();
};
