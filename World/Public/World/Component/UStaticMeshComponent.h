#pragma once
#include "World/Component/UMeshComponent.h"
#include "Core/Archive/FArchive.h"
#include "Asset/UMaterial.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/Component/UStaticMeshComponent.generated.h"

UCLASS()
class UStaticMeshComponent : public UMeshComponent {
public:
    GENERATED_TYPES(UStaticMeshComponent)

public:
    UStaticMeshComponent();
    ~UStaticMeshComponent() override = default;

public:
    GENERATED_BODY(UStaticMeshComponent)

    FAssetHandle GetMaterialHandle() const;
    FAssetHandle GetPipelineHandle() const;

    void SetMeshHandle(FAssetHandle InHandle) override;
    void SetMaterialHandle(FAssetHandle InHandle);
    void SetEditorMaterialHandle(FAssetHandle InHandle);
    void SetPipelineHandle(FAssetHandle InHandle);

    void OnRegister() override;
    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;

private:
    void Serialize(FArchive& Archive) override;

    void EnsureDefaultRenderAssets();

private:
    UPROPERTY(Edit, Serialize, Asset, AssetClass = UMaterial, Getter = GetMaterialHandle, Setter = SetEditorMaterialHandle, AssetGuid = mMaterialAssetGuid, AssetPath = mMaterialAssetPath, Category = "Rendering", DisplayName = "Material")
    FAssetHandle mMaterialHandle{};
    UPROPERTY(Edit, Serialize, Asset, AssetClass = UPipeline, Getter = GetPipelineHandle, Setter = SetPipelineHandle, AssetGuid = mPipelineAssetGuid, AssetPath = mPipelineAssetPath, Category = "Rendering", DisplayName = "Pipeline")
    FAssetHandle mPipelineHandle{};

    FAssetPath mMaterialAssetPath{};
    FAssetPath mPipelineAssetPath{};

    FGuid mMaterialAssetGuid{};
    FGuid mPipelineAssetGuid{};
};
