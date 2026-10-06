#pragma once
#include "Asset/FMaterialGroup.h"
#include "Asset/UMaterial.h"

#include <functional>

class USurfaceOpaque : public UMaterial {
public:
    using FTextureResolver = std::function<FAssetHandle(const std::filesystem::path& TexturePath)>;

    USurfaceOpaque() = default;
    virtual ~USurfaceOpaque() = default;

    USurfaceOpaque(const USurfaceOpaque&) = delete;
    USurfaceOpaque& operator=(const USurfaceOpaque&) = delete;

    USurfaceOpaque(USurfaceOpaque&&) noexcept = default;
    USurfaceOpaque& operator=(USurfaceOpaque&&) noexcept = default;

public:
    JG_DECLARE_DERIVED_TYPEINFO(USurfaceOpaque, UMaterial);

    bool Initialize(const std::filesystem::path& MtlPath, const FTextureResolver& TextureResolver);
    void BuildGPUData(FMaterialGPUSlot& OutSlot) const override;
    void BuildGPUData(Uint32 GroupIndex, FMaterialGPUSlot& OutSlot) const override;
    FMaterialChunkSignature BuildChunkSignature() const override;
    FMaterialChunkSignature BuildChunkSignature(Uint32 GroupIndex) const override;
    Uint32 GetGPUDataCount() const override;
    std::optional<Uint32> FindGroupIndex(FName Name) const override;

    const TArray<FMaterialGroup>& GetGroups() const;
    bool ModifyGroup(Uint32 GroupIndex, const std::function<void(FMaterialGroup&)>& Modifier);

private:
    void Serialize(FArchive& Ar) override;
    void Reset();

private:
    TArray<FMaterialGroup> mGroups{};
};
