#include "FRenderTargetPool.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <algorithm>

namespace Renderer {

    Asset::UTexture2D* Renderer::FPooledRenderTarget::GetTexture() const {
        return mTexture.get();
    }

    ID3D11DepthStencilView* Renderer::FPooledRenderTarget::GetReadOnlyDepthView() const {
        return mReadOnlyDepthView.Get();
    }

    ID3D11ShaderResourceView* Renderer::FPooledRenderTarget::GetStencilResource() const {
        return mStencilResource.Get();
    }

    Renderer::FRenderTargetLease::FRenderTargetLease(Renderer::FRenderTargetPool* Pool, Renderer::FPooledRenderTarget* Target)
        : mPool(Pool), mTarget(Target) {
    }

    Renderer::FRenderTargetLease::~FRenderTargetLease() {
        Reset();
    }

    Renderer::FRenderTargetLease::FRenderTargetLease(Renderer::FRenderTargetLease&& Other) noexcept
        : mPool(Other.mPool), mTarget(Other.mTarget) {
        Other.mPool = nullptr;
        Other.mTarget = nullptr;
    }

    Renderer::FRenderTargetLease& Renderer::FRenderTargetLease::operator=(Renderer::FRenderTargetLease&& Other) noexcept {
        if (this != &Other) {
            Reset();
            mPool = Other.mPool;
            mTarget = Other.mTarget;
            Other.mPool = nullptr;
            Other.mTarget = nullptr;
        }

        return *this;
    }

    Renderer::FPooledRenderTarget* Renderer::FRenderTargetLease::Get() const {
        return mTarget;
    }

    Asset::UTexture2D* Renderer::FRenderTargetLease::GetTexture() const {
        return mTarget != nullptr ? mTarget->GetTexture() : nullptr;
    }

    Renderer::FRenderTargetLease::operator bool() const {
        return mTarget != nullptr;
    }

    void Renderer::FRenderTargetLease::Reset() {
        if (mPool != nullptr && mTarget != nullptr) {
            mPool->Release(mTarget);
        }

        mPool = nullptr;
        mTarget = nullptr;
    }

    Renderer::FRenderTargetPool::~FRenderTargetPool() = default;

    Renderer::FRenderTargetLease Renderer::FRenderTargetPool::Acquire(ID3D11Device* Device, const Renderer::FRenderTargetDesc& Desc) {
        for (const auto& Target : mTargets) {
            if (!Target->mInUse && Target->mDesc == Desc) {
                Target->mInUse = true;
                Target->mLastUsedFrame = mFrame;

                return {this, Target.get()};
            }
        }

        if (Device == nullptr) {
            return {};
        }

        auto Target{std::make_unique<Renderer::FPooledRenderTarget>()};
        auto Texture{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (!Texture->Configure(Desc.mWidth, Desc.mHeight, Desc.mFormat, Desc.mUsage) || !Texture->CreateResources(Device)) {
            return {};
        }

        const bool StencilViews{Desc.mFormat == Asset::ETextureFormat::D24S8 && (Desc.mUsage & Asset::ETextureUsage::DepthStencil) != Asset::ETextureUsage::None && (Desc.mUsage & Asset::ETextureUsage::ShaderResource) != Asset::ETextureUsage::None};

        if (StencilViews) {
            D3D11_DEPTH_STENCIL_VIEW_DESC ReadOnlyDescription{};
            Texture->GetDepthStencilView()->GetDesc(&ReadOnlyDescription);
            ReadOnlyDescription.Flags = D3D11_DSV_READ_ONLY_DEPTH | D3D11_DSV_READ_ONLY_STENCIL;

            D3D11_SHADER_RESOURCE_VIEW_DESC StencilDescription{};
            StencilDescription.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
            StencilDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            StencilDescription.Texture2D.MipLevels = 1;

            if (FAILED(Device->CreateDepthStencilView(Texture->GetTexture(), &ReadOnlyDescription, Target->mReadOnlyDepthView.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(Texture->GetTexture(), &StencilDescription, Target->mStencilResource.GetAddressOf()))) {
                return {};
            }
        }

        Target->mDesc = Desc;
        Target->mTexture = std::move(Texture);
        Target->mInUse = true;
        Target->mLastUsedFrame = mFrame;

        Renderer::FPooledRenderTarget* Result{Target.get()};
        mTargets.push_back(std::move(Target));

        return {this, Result};
    }

    void Renderer::FRenderTargetPool::Tick() {
        ++mFrame;

        std::erase_if(mTargets, [this](const std::unique_ptr<Renderer::FPooledRenderTarget>& Target) {
            return !Target->mInUse && mFrame - Target->mLastUsedFrame > MaxIdleFrames;
        });
    }

    void Renderer::FRenderTargetPool::Clear() {
        std::erase_if(mTargets, [](const std::unique_ptr<Renderer::FPooledRenderTarget>& Target) {
            return !Target->mInUse;
        });
    }

    std::size_t Renderer::FRenderTargetPool::GetResourceBytes() const {
        std::size_t Bytes{};

        for (const auto& Target : mTargets) {
            Bytes += Target->mTexture->GetResourceBytes();
        }

        return Bytes;
    }

    void Renderer::FRenderTargetPool::Release(Renderer::FPooledRenderTarget* Target) {
        Target->mInUse = false;
    }

}
