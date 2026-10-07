#pragma once
#include "RenderCore/FMaterialChunkSignature.h"
#include "RenderCore/FMaterialGPUData.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "Asset/UAsset.h"
#include "CoreUObject/Asset/IAssetRegistry.h"

#include <optional>
#include <span>

class UMaterial : public UAsset {
public:
    UMaterial() = default;
    virtual ~UMaterial() = default;

    UMaterial(const UMaterial&) = delete;
    UMaterial& operator=(const UMaterial&) = delete;

    UMaterial(UMaterial&&) noexcept = default;
    UMaterial& operator=(UMaterial&&) noexcept = default;

public:
    JG_DECLARE_ABSTRACT_DERIVED_TYPEINFO(UMaterial, UAsset);

    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const = 0;
    virtual void BuildGPUData(Uint32 GroupIndex, FMaterialGPUSlot& OutSlot) const;
    virtual FMaterialChunkSignature BuildChunkSignature() const;
    virtual FMaterialChunkSignature BuildChunkSignature(Uint32 GroupIndex) const;
    virtual void Finalize(const IAssetRegistry* Query);

    virtual Uint32 GetGPUDataCount() const;
    virtual EMaterialBlendMode GetBlendMode(Uint32 GroupIndex = 0) const;

    virtual std::optional<Uint32> FindGroupIndex(FName Name) const;

    void MarkGPUDataDirty();
    Uint64 GetRenderRevision() const;

protected:
    void Serialize(FArchive& Ar) override;

private:
    Uint64 mRenderRevision{1};
};
