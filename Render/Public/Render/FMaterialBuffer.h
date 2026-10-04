#pragma once

#include <d3d11.h>
#include <wrl/client.h>
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "RenderCore/FMaterialGPUData.h"

class UMaterial;

class FMaterialBuffer {
private:
    struct FMaterialEntry {
        FAssetHandle mHandle{};
        Uint64 mRevision{};
        TArray<Uint32> mIndices{};
    };

public:
    FMaterialBuffer() = default;
    ~FMaterialBuffer() = default;

    FMaterialBuffer(const FMaterialBuffer&) = delete;
    FMaterialBuffer& operator=(const FMaterialBuffer&) = delete;
    FMaterialBuffer(FMaterialBuffer&&) = delete;
    FMaterialBuffer& operator=(FMaterialBuffer&&) = delete;

public:
    bool Initialize(ID3D11Device* Device, Uint32 MaxMaterialCount = 4096);
    bool Synchronize(const IAssetRegistry& Registry, ID3D11DeviceContext* Context);
    Uint32 GetMaterialIndex(const UMaterial& Material, Uint32 GroupIndex = 0) const;
    Uint64 GetRevision() const;
    ID3D11ShaderResourceView* GetSRV() const;
    void Reset();

private:
    void ReleaseMaterial(FMaterialEntry& Entry);

private:
    TMap<FGuid, FMaterialEntry> mMaterials{};
    TArray<Uint32> mFreeIndices{};
    Uint64 mRevision{1};
    Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mSrv{};
};
