#include "RenderTarget.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Types.h"

namespace Renderer {

    Renderer::FDXRenderTarget::FDXRenderTarget() = default;

    Renderer::FDXRenderTarget::~FDXRenderTarget() = default;

    bool Renderer::FDXRenderTarget::Resize(ID3D11Device* Device, Uint32 Width, Uint32 Height) {
        if (Device == nullptr || Width == 0 || Height == 0 || Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
            return false;
        }

        if (Width == mWidth && Height == mHeight && mColor != nullptr && mColor->HasResources()) {
            Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};

            mColor->GetTexture()->GetDevice(ResourceDevice.GetAddressOf());

            if (ResourceDevice.Get() == Device) {
                return true;
            }
        }

        auto Color{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (!Color->Configure(Width, Height, Asset::ETextureFormat::RGBA8, Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget) || !Color->CreateResources(Device)) {
            return false;
        }

        mColor = std::move(Color);
        mWidth = Width;
        mHeight = Height;

        return true;
    }

    Uint32 Renderer::FDXRenderTarget::GetWidth() const {
        return mWidth;
    }

    Uint32 Renderer::FDXRenderTarget::GetHeight() const {
        return mHeight;
    }

    void* Renderer::FDXRenderTarget::GetNativeTexture() const {
        return mColor != nullptr ? mColor->GetShaderResourceView() : nullptr;
    }

    ID3D11RenderTargetView* Renderer::FDXRenderTarget::GetRenderTargetView() const {
        return mColor != nullptr ? mColor->GetRenderTargetView() : nullptr;
    }

    Asset::UTexture2D* Renderer::FDXRenderTarget::GetOutputColor() const {
        return mColor.get();
    }

    std::size_t Renderer::FDXRenderTarget::GetResourceBytes() const {
        return mColor != nullptr ? mColor->GetResourceBytes() : 0;
    }

}
