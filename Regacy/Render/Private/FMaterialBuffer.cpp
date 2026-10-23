#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FMaterialBuffer.h"
#include "Asset/UMaterial.h"

bool FMaterialBuffer::Initialize(ID3D11Device* Device, Uint32 MaxMaterialCount) {
    Reset();

    if (Device == nullptr || MaxMaterialCount == 0 || MaxMaterialCount > UINT32_MAX / MaterialGpuStride) {
        return false;
    }

    D3D11_BUFFER_DESC Description{};

    Description.ByteWidth = MaterialGpuStride * MaxMaterialCount;
    Description.Usage = D3D11_USAGE_DEFAULT;
    Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Description.StructureByteStride = MaterialGpuStride;

    if (FAILED(Device->CreateBuffer(&Description, nullptr, mBuffer.GetAddressOf()))) {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC ViewDescription{};

    ViewDescription.Format = DXGI_FORMAT_UNKNOWN;
    ViewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    ViewDescription.Buffer.NumElements = MaxMaterialCount;

    if (FAILED(Device->CreateShaderResourceView(mBuffer.Get(), &ViewDescription, mSrv.GetAddressOf()))) {
        Reset();
        return false;
    }

    mFreeIndices.reserve(MaxMaterialCount);

    for (Uint32 Index{}; Index < MaxMaterialCount; ++Index) {
        mFreeIndices.push_back(MaxMaterialCount - Index - 1);
    }

    return true;
}

bool FMaterialBuffer::Synchronize(ID3D11DeviceContext* Context) {
    if (Context == nullptr || mBuffer == nullptr) {
        return false;
    }

    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    std::erase_if(mMaterials, [this, Registry](auto& Pair) {
        const UMaterial* Material{Registry != nullptr ? Registry->ResolveAsset<UMaterial>(Pair.second.mHandle) : nullptr};

        if (Material != nullptr && Material->GetGuid() == Pair.first) {
            return false;
        }

        ReleaseMaterial(Pair.second);

        return true;
    });

    if (Registry == nullptr) {
        return false;
    }

    bool Complete{true};

    for (FAssetHandle Handle : Registry->GetAssetHandles(*UMaterial::StaticClass())) {
        const UMaterial* Material{Registry->ResolveAsset<UMaterial>(Handle)};

        if (Material == nullptr) {
            continue;
        }

        auto Position{mMaterials.find(Material->GetGuid())};

        if (Position == mMaterials.end()) {
            if (mFreeIndices.empty()) {
                Complete = false;
                continue;
            }

            FMaterialEntry Entry{};

            Entry.mHandle = Handle;

            Entry.mIndex = mFreeIndices.back();
            mFreeIndices.pop_back();

            Position = mMaterials.emplace(Material->GetGuid(), std::move(Entry)).first;
            ++mRevision;
        }

        FMaterialEntry& Entry{Position->second};

        const FRenderAssetStamp Stamp{Material->GetHandle(), Material->GetRenderRevision()};

        if (Entry.mStamp == Stamp) {
            continue;
        }

        FMaterialGPUSlot Slot{};

        Material->BuildGPUData(Slot);

        D3D11_BOX Box{};

        Box.left = Entry.mIndex * MaterialGpuStride;
        Box.right = Box.left + MaterialGpuStride;
        Box.bottom = 1;
        Box.back = 1;
        Context->UpdateSubresource(mBuffer.Get(), 0, &Box, Slot.mData.data(), 0, 0);

        Entry.mStamp = Stamp;
    }

    return Complete;
}

Uint32 FMaterialBuffer::GetMaterialIndex(const UMaterial& Material) const {
    const auto Position{mMaterials.find(Material.GetGuid())};

    return Position != mMaterials.end() ? Position->second.mIndex : UINT32_MAX;
}

void FMaterialBuffer::BuildIndexRemapping(const FMaterialBuffer& Target, TMap<Uint32, Uint32>& OutIndices) const {
    OutIndices.clear();

    for (const auto& [Guid, Source] : mMaterials) {
        const auto Position{Target.mMaterials.find(Guid)};

        if (Position == Target.mMaterials.end()) {
            continue;
        }

        OutIndices.emplace(Source.mIndex, Position->second.mIndex);
    }
}

Uint64 FMaterialBuffer::GetRevision() const {
    return mRevision;
}

ID3D11ShaderResourceView* FMaterialBuffer::GetSRV() const {
    return mSrv.Get();
}

void FMaterialBuffer::ReleaseMaterial(FMaterialEntry& Entry) {
    mFreeIndices.push_back(Entry.mIndex);
    ++mRevision;
}

void FMaterialBuffer::Reset() {
    mMaterials.clear();
    mFreeIndices.clear();
    mSrv.Reset();
    mBuffer.Reset();
    ++mRevision;
}
