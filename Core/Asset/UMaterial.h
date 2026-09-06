#pragma once

#include "../Base/UObject.h"
#include "FMaterialGPUData.h"

#include <d3d11.h>

class FMaterialBuffer;

class UMaterial : public UObject {
public:
    UMaterial() = default;
    virtual ~UMaterial() = default;

    UMaterial(const UMaterial&) = delete;
    UMaterial& operator=(const UMaterial&) = delete;

    UMaterial(UMaterial&&) noexcept = default;
    UMaterial& operator=(UMaterial&&) noexcept = default;

public:
	JG_DECLARE_DERIVED_TYPEINFO(UMaterial, UObject);

    virtual bool Initialize(ID3D11Device* Device);
    virtual void BuildGPUData(FMaterialGPUSlot& OutSlot) const;

    uint32 GetGPUIndex() const { return GPUIndex; }

protected:
    void MarkGPUDataDirty() { bGPUDataDirty = true; }

private:
    friend class FMaterialBuffer;

    uint32 GPUIndex{ UINT32_MAX };
    bool bGPUDataDirty{ true };
};