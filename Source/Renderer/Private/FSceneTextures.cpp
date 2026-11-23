#include "FSceneTextures.h"

#include <algorithm>

namespace Renderer {

    Renderer::FSceneTextures::FSceneTextures(Renderer::FRenderTargetPool& Pool, ID3D11Device* Device, Asset::UTexture2D& OutputColor, bool Deferred)
        : mPool(&Pool), mDevice(Device), mOutputColor(&OutputColor), mDeferred(Deferred) {
        const Uint32 Width{OutputColor.GetSizeX()};
        const Uint32 Height{OutputColor.GetSizeY()};
        const Asset::ETextureUsage ColorUsage{Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget};

        mSceneColor = Pool.Acquire(Device, {Width, Height, Asset::ETextureFormat::RGBA8, ColorUsage});
        mPostProcessColor = Pool.Acquire(Device, {Width, Height, Asset::ETextureFormat::RGBA8, ColorUsage});
        mDepth = Pool.Acquire(Device, {Width, Height, Asset::ETextureFormat::D24S8, Asset::ETextureUsage::DepthStencil | Asset::ETextureUsage::ShaderResource});

        if (Deferred) {
            for (auto& Attachment : mGBuffer) {
                Attachment = Pool.Acquire(Device, {Width, Height, Asset::ETextureFormat::RGBA16Float, ColorUsage});
            }
        }
    }

    bool Renderer::FSceneTextures::IsValid() const {
        return mSceneColor && mPostProcessColor && mDepth && (!mDeferred || std::all_of(mGBuffer.begin(), mGBuffer.end(), [](const Renderer::FRenderTargetLease& Attachment) {
            return static_cast<bool>(Attachment);
        }));
    }

    Uint32 Renderer::FSceneTextures::GetWidth() const {
        return mOutputColor->GetSizeX();
    }

    Uint32 Renderer::FSceneTextures::GetHeight() const {
        return mOutputColor->GetSizeY();
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetSceneColor() const {
        return mSceneColor.GetTexture();
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetPostProcessColor() const {
        return mPostProcessColor.GetTexture();
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetOutputColor() const {
        return mOutputColor;
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetSceneDepth() const {
        return mDepth.GetTexture();
    }

    ID3D11DepthStencilView* Renderer::FSceneTextures::GetDepthView() const {
        return mDepth ? mDepth.GetTexture()->GetDepthStencilView() : nullptr;
    }

    ID3D11DepthStencilView* Renderer::FSceneTextures::GetReadOnlyDepthView() const {
        return mDepth ? mDepth.Get()->GetReadOnlyDepthView() : nullptr;
    }

    ID3D11ShaderResourceView* Renderer::FSceneTextures::GetStencilResource() const {
        return mDepth ? mDepth.Get()->GetStencilResource() : nullptr;
    }

    bool Renderer::FSceneTextures::HasGBuffer() const {
        return mDeferred;
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetGBuffer(Index_t Index) const {
        return Index < mGBuffer.size() ? mGBuffer[Index].GetTexture() : nullptr;
    }

    Asset::UTexture2D* Renderer::FSceneTextures::GetDecalNormalSource(bool Accumulated) {
        Renderer::FRenderTargetLease& NormalSource{Accumulated ? mDecalNormalHistory : mDecalNormalSource};
        const Asset::UTexture2D* Normals{GetGBuffer(Renderer::GBuffer::NormalAttachment)};

        if (!NormalSource && Normals != nullptr) {
            NormalSource = mPool->Acquire(mDevice, {Normals->GetSizeX(), Normals->GetSizeY(), Normals->GetFormat(), Asset::ETextureUsage::ShaderResource});
        }

        return NormalSource.GetTexture();
    }

}
