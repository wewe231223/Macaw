#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UMeshComponent.h"
#include "Core/Archive/FArchive.h"
#include "Asset/UMaterial.h"
#include "Asset/Pipeline/UPipeline.h"

class FProperty;

class UStaticMeshComponent : public UMeshComponent {
public:
    ReflectTypes(UStaticMeshComponent, UMeshComponent);

public:
    UStaticMeshComponent();
    ~UStaticMeshComponent() override = default;

public:
    ReflectClass(UStaticMeshComponent, EClassFlags::None);

    FAssetHandle GetMaterialHandle() const;
    FAssetHandle GetPipelineHandle() const;

    void SetMaterialHandle(FAssetHandle InHandle);
    void SetEditorMaterialHandle(FAssetHandle InHandle);
    void SetPipelineHandle(FAssetHandle InHandle);

    void OnRegister() override;
    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;

private:
    static void SerializeMaterialHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);
    static void SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);

private:
    ReflectAccessor(mMaterialReference, GetMaterialHandle, SetEditorMaterialHandle, "MaterialHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializeMaterialHandleProperty, ReflectEditor("Rendering", "Material"), ReflectAsset(UMaterial));
    FAssetReference mMaterialReference{};
    ReflectAccessor(mPipelineReference, GetPipelineHandle, SetPipelineHandle, "PipelineHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializePipelineHandleProperty, ReflectEditor("Rendering", "Pipeline"), ReflectAsset(UPipeline));
    FAssetReference mPipelineReference{};

    ReflectMembers();
};
