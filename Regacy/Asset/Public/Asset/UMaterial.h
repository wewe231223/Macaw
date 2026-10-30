#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FMaterialChunkSignature.h"
#include "RenderCore/FMaterialGPUData.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "Asset/UAsset.h"
#include "CoreUObject/Asset/IAssetRegistry.h"

enum class EMaterialShader : Uint8 {
    Surface,
    Base,
    Textured,
    BaseSolid,
    Alternate,
    AlternateSolid,
    Sky,
    Gizmo
};

ReflectEnum(EMaterialShader, ReflectEnumValue(EMaterialShader, Surface), ReflectEnumValue(EMaterialShader, Base), ReflectEnumValue(EMaterialShader, Textured), ReflectEnumValue(EMaterialShader, BaseSolid), ReflectEnumValue(EMaterialShader, Alternate), ReflectEnumValue(EMaterialShader, AlternateSolid), ReflectEnumValue(EMaterialShader, Sky), ReflectEnumValue(EMaterialShader, Gizmo));

class UMaterial : public UAsset {
public:
    ReflectTypes(UMaterial, UAsset);

public:
    UMaterial();
    ~UMaterial() override = default;
    UMaterial(const UMaterial&) = delete;

    UMaterial& operator=(const UMaterial&) = delete;

    UMaterial(UMaterial&&) noexcept = default;

    UMaterial& operator=(UMaterial&&) noexcept = default;

public:
    ReflectClass(UMaterial, EClassFlags::Abstract);

    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const = 0;
    virtual FMaterialChunkSignature BuildChunkSignature() const = 0;
    virtual void Finalize(const IAssetRegistry* Registry);
    virtual EMaterialBlendMode GetBlendMode() const;

    EMaterialShader GetShader() const;
    void SetShader(EMaterialShader Shader);
    void MarkGPUDataDirty();
    Uint64 GetRenderRevision() const;

private:
    ReflectAccessor(mShader, GetShader, SetShader, "Shader", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mShader), ReflectEditor("Material", "Shader"));

    EMaterialShader mShader{EMaterialShader::Surface};
    Uint64 mRenderRevision{1};

    ReflectMembers();
};
