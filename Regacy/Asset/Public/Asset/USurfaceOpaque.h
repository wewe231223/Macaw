#pragma once

#include "Asset/FSurfaceParameters.h"
#include "Asset/UMaterial.h"

class USurfaceOpaque : public UMaterial {
public:
    ReflectTypes(USurfaceOpaque, UMaterial);

public:
    USurfaceOpaque();
    ~USurfaceOpaque() override = default;
    USurfaceOpaque(const USurfaceOpaque&) = delete;

    USurfaceOpaque& operator=(const USurfaceOpaque&) = delete;

    USurfaceOpaque(USurfaceOpaque&&) noexcept = default;

    USurfaceOpaque& operator=(USurfaceOpaque&&) noexcept = default;

public:
    ReflectClass(USurfaceOpaque);

    const FSurfaceParameters& GetParameters() const;
    bool SetParameters(const FSurfaceParameters& Parameters);
    void BuildGPUData(FMaterialGPUSlot& OutSlot) const override;
    FMaterialChunkSignature BuildChunkSignature() const override;
    EMaterialBlendMode GetBlendMode() const override;
    void Finalize(const IAssetRegistry* Registry) override;

private:
    ReflectAccessor(mParameters, GetParameters, SetParameters, "Parameters", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mParameters), ReflectEditor("Material", "Surface"));

    FSurfaceParameters mParameters{};

    ReflectMembers();
};
