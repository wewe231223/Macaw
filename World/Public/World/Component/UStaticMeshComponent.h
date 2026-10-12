#pragma once
#include "World/Component/UMeshComponent.h"
#include "Core/Archive/FArchive.h"
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
    void SetPipelineHandle(FAssetHandle InHandle);

    void OnRegister() override;
    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;

private:
    void Serialize(FArchive& Archive) override;

    void EnsureDefaultRenderAssets();

private:
    FAssetHandle mMaterialHandle{};
    FAssetHandle mPipelineHandle{};

    FAssetPath mMaterialAssetPath{};
    FAssetPath mPipelineAssetPath{};

    FGuid mMaterialAssetGuid{};
    FGuid mPipelineAssetGuid{};
};
