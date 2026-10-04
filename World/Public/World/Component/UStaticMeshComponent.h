#pragma once
#include "World/Component/UMeshComponent.h"
#include "Core/Archive/FArchive.h"

class UStaticMeshComponent : public UMeshComponent {
public:
    UStaticMeshComponent() = default;
    ~UStaticMeshComponent() override = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(UStaticMeshComponent, UMeshComponent)

    FAssetHandle GetMaterialHandle() const;
    FAssetHandle GetPipelineHandle() const;

    void SetMeshHandle(FAssetHandle InHandle) override;
    void SetMaterialHandle(FAssetHandle InHandle);
    void SetPipelineHandle(FAssetHandle InHandle);

    void OnRegister() override;
    void OnUnregister() override;

    virtual void MakeRender(FActorProbe& OutProbe) const override;

private:
    void Serialize(FArchive& Archive) override;

    void EnsureDefaultRenderAssets();
    void OnRenderStateChanged() override;

private:
    FAssetHandle mMaterialHandle{};
    FAssetHandle mPipelineHandle{};

    FAssetPath mMaterialAssetPath{};
    FAssetPath mPipelineAssetPath{};

    FGuid mMaterialAssetGuid{};
    FGuid mPipelineAssetGuid{};
};
