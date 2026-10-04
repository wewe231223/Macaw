#include "pch.h"
#include "Render/FRenderAssetResources.h"
#include "Asset/UMesh.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include <DirectXTex.h>

bool FRenderAssetResources::Initialize(ID3D11Device* Device) {
    Reset();
    if (!mMaterials.Initialize(Device)) {
        return false;
    }
    mDevice = Device;
    return true;
}

void FRenderAssetResources::Prune(const IAssetRegistry& Registry) {
    TSet<FGuid> LiveAssets{};
    for (FAssetHandle Handle : Registry.GetAssetHandles(*UAsset::StaticTypeInfo())) {
        const UAsset* Asset{Registry.ResolveAsset<UAsset>(Handle)};
        if (Asset != nullptr) {
            LiveAssets.insert(Asset->GetGuid());
        }
    }
    const auto Expired{[&LiveAssets](const auto& Pair) {
        return !LiveAssets.contains(Pair.first);
    }};
    std::erase_if(mMeshes, Expired);
    std::erase_if(mPipelines, Expired);
    std::erase_if(mTextures, Expired);
    std::erase_if(mFonts, Expired);
}

void FRenderAssetResources::Reset() {
    mMeshes.clear();
    mPipelines.clear();
    mTextures.clear();
    mFonts.clear();
    mMaterials.Reset();
    mDevice = nullptr;
}

const FMeshRenderResource* FRenderAssetResources::GetMesh(const UMesh& Mesh) {
    if (mDevice == nullptr) {
        return nullptr;
    }
    FMeshEntry& Entry{mMeshes[Mesh.GetGuid()]};
    if (Entry.mRevision != Mesh.GetRenderRevision()) {
        FMeshRenderResource Resource{};
        if (!Resource.Initialize(mDevice, Mesh)) {
            return nullptr;
        }
        Entry.mResource = std::move(Resource);
        Entry.mRevision = Mesh.GetRenderRevision();
    }
    return &Entry.mResource;
}

const FPipelineRenderResource* FRenderAssetResources::GetPipeline(const UPipeline& Pipeline) {
    if (mDevice == nullptr) {
        return nullptr;
    }
    FPipelineEntry& Entry{mPipelines[Pipeline.GetGuid()]};
    if (Entry.mRevision != Pipeline.GetRenderRevision()) {
        FPipelineRenderResource Resource{};
        if (!Resource.Initialize(mDevice, Pipeline)) {
            return nullptr;
        }
        Entry.mResource = std::move(Resource);
        Entry.mRevision = Pipeline.GetRenderRevision();
    }
    return &Entry.mResource;
}

ID3D11ShaderResourceView* FRenderAssetResources::GetTexture(const UTexture& Texture) {
    const DirectX::ScratchImage* Source{Texture.GetSourceImage()};
    if (mDevice == nullptr || Source == nullptr || Source->GetImageCount() == 0) {
        return nullptr;
    }
    FTextureEntry& Entry{mTextures[Texture.GetGuid()]};
    if (Entry.mRevision != Texture.GetRenderRevision()) {
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View{};
        if (FAILED(DirectX::CreateShaderResourceView(mDevice, Source->GetImages(), Source->GetImageCount(), Source->GetMetadata(), View.GetAddressOf()))) {
            return nullptr;
        }
        Entry.mView = std::move(View);
        Entry.mRevision = Texture.GetRenderRevision();
    }
    return Entry.mView.Get();
}

ID3D11ShaderResourceView* FRenderAssetResources::GetFontAtlas(const UFont& Font, ID3D11DeviceContext* Context) {
    const std::span<const Uint8> Pixels{Font.GetAtlasPixels()};
    const Uint32 Width{Font.GetAtlasWidth()};
    const Uint32 Height{Font.GetAtlasHeight()};
    if (mDevice == nullptr || Context == nullptr || Width == 0 || Height == 0 || Pixels.size() != static_cast<std::size_t>(Width) * Height) {
        return nullptr;
    }
    FFontEntry& Entry{mFonts[Font.GetGuid()]};
    if (Entry.mTexture == nullptr || Entry.mWidth != Width || Entry.mHeight != Height) {
        FFontEntry Replacement{};
        D3D11_TEXTURE2D_DESC Description{};
        Description.Width = Width;
        Description.Height = Height;
        Description.MipLevels = 1;
        Description.ArraySize = 1;
        Description.Format = DXGI_FORMAT_R8_UNORM;
        Description.SampleDesc.Count = 1;
        Description.Usage = D3D11_USAGE_DEFAULT;
        Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA InitialData{};
        InitialData.pSysMem = Pixels.data();
        InitialData.SysMemPitch = Width;
        if (FAILED(mDevice->CreateTexture2D(&Description, &InitialData, Replacement.mTexture.GetAddressOf())) || FAILED(mDevice->CreateShaderResourceView(Replacement.mTexture.Get(), nullptr, Replacement.mView.GetAddressOf()))) {
            return nullptr;
        }
        Replacement.mWidth = Width;
        Replacement.mHeight = Height;
        Replacement.mRevision = Font.GetAtlasRevision();
        Entry = std::move(Replacement);
    } else if (Entry.mRevision != Font.GetAtlasRevision()) {
        Context->UpdateSubresource(Entry.mTexture.Get(), 0, nullptr, Pixels.data(), Width, 0);
        Entry.mRevision = Font.GetAtlasRevision();
    }
    return Entry.mView.Get();
}

FMaterialBuffer& FRenderAssetResources::GetMaterialBuffer() {
    return mMaterials;
}
