#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "RenderCore/FMaterialChunkSignature.h"
#include "RenderCore/FMaterialGPUData.h"
#include "RenderCore/FMaterialBlendMode.h"
#include "Asset/UAsset.h"
#include "CoreUObject/Asset/IAssetRegistry.h"

#include <optional>

struct FResolvedMaterialData;

class UMaterial : public UAsset {
public:
    ReflectTypes(UMaterial, UAsset);

public:
    UMaterial();
    virtual ~UMaterial();

    UMaterial(const UMaterial&) = delete;
    UMaterial& operator=(const UMaterial&) = delete;

    UMaterial(UMaterial&&) noexcept = default;
    UMaterial& operator=(UMaterial&&) noexcept = default;

public:
    ReflectClass(UMaterial, EClassFlags::Abstract);

    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const;
    virtual void BuildGPUData(Uint32 GroupIndex, FMaterialGPUSlot& OutSlot) const;
    virtual FMaterialChunkSignature BuildChunkSignature() const;
    virtual FMaterialChunkSignature BuildChunkSignature(Uint32 GroupIndex) const;
    virtual void Finalize(const IAssetRegistry* Query);
    virtual bool BuildResolvedData(const IAssetRegistry& Registry, FResolvedMaterialData& OutData) const;

    virtual Uint32 GetGPUDataCount() const;
    virtual EMaterialBlendMode GetBlendMode(Uint32 GroupIndex = 0) const;

    virtual std::optional<Uint32> FindGroupIndex(FName Name) const;
    virtual FName GetGroupName(Uint32 GroupIndex) const;

    void MarkGPUDataDirty();
    Uint64 GetRenderRevision() const;

protected:
    void Serialize(FArchive& Ar) override;

private:
    Uint64 mRenderRevision{1};
};
