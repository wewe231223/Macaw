#pragma once

#include "FMaterialGPUData.h"

#include <d3d11.h>
#include <wrl/client.h>
#include "Asset/UMaterial.h"

struct FMaterialBufferEntry {
    UMaterial* mMaterial{nullptr};
    Uint32 mGroupIndex{0};
};

class FMaterialBuffer {
public:
    FMaterialBuffer() = default;
    ~FMaterialBuffer() = default;

    FMaterialBuffer(const FMaterialBuffer&) = delete;
    FMaterialBuffer& operator=(const FMaterialBuffer&) = delete;

    FMaterialBuffer(FMaterialBuffer&&) = delete;
    FMaterialBuffer& operator=(FMaterialBuffer&&) = delete;

public:
    bool Initialize(ID3D11Device* Device, Uint32 MaxMaterialCount);

    bool RegisterMaterial(UMaterial* Material);
    void UnregisterMaterial(UMaterial* Material);

    void Flush(ID3D11DeviceContext* DeviceContext);

    ID3D11Buffer* GetBuffer() const;

    ID3D11ShaderResourceView* const* GetSRV() const;

    Uint32 GetMaxMaterialCount() const;

    void Reset();

private:
    Uint32 AllocateSlot();
    void ReleaseSlot(Uint32 Index);

private:
    Uint32 mMaxMaterialCount{0};

    TArray<FMaterialGPUSlot> mSlots{};
    TArray<FMaterialBufferEntry> mMaterials{};
    TArray<Uint32> mFreeIndices{};

    Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSrv{};
};
