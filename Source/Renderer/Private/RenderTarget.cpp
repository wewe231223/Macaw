/**
 * @file RenderTarget.cpp
 * @brief Direct3D 11 오프스크린 렌더 타깃을 구현합니다.
 * @ingroup Renderer
 */

#include "RenderTarget.h"
#include "CoreUObject/Reflection/NewObject.h"

namespace Renderer {

    /**
     * @brief FDXRenderTarget의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Renderer::FDXRenderTarget::FDXRenderTarget() = default;

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Renderer::FDXRenderTarget::~FDXRenderTarget() = default;

    /**
     * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
     */
    bool Renderer::FDXRenderTarget::Resize(ID3D11Device* Device, Core::Uint32 Width, Core::Uint32 Height) {
        if (Device == nullptr || Width == 0 || Height == 0 || Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
            return false;
        }

        if (Width == mWidth && Height == mHeight && mColor != nullptr && mDepth != nullptr && mSceneColor != nullptr && mPostProcessColor != nullptr && mColor->HasResources() && mDepth->HasResources() && mSceneColor->HasResources() && mPostProcessColor->HasResources()) {
            Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};
            mColor->GetTexture()->GetDevice(ResourceDevice.GetAddressOf());

            if (ResourceDevice.Get() == Device) {
                return true;
            }
        }

        auto Color{CoreUObject::NewObject<Asset::UTexture2D>()};
        auto Depth{CoreUObject::NewObject<Asset::UTexture2D>()};
        auto SceneColor{CoreUObject::NewObject<Asset::UTexture2D>()};
        auto PostProcessColor{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (!Color->Configure(Width, Height, Asset::ETextureFormat::RGBA8, Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget) || !Depth->Configure(Width, Height, Asset::ETextureFormat::D24S8, Asset::ETextureUsage::DepthStencil | Asset::ETextureUsage::ShaderResource) || !SceneColor->Configure(Width, Height, Asset::ETextureFormat::RGBA8, Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget) || !PostProcessColor->Configure(Width, Height, Asset::ETextureFormat::RGBA8, Asset::ETextureUsage::ShaderResource | Asset::ETextureUsage::RenderTarget) || !Color->CreateResources(Device) || !Depth->CreateResources(Device) || !SceneColor->CreateResources(Device) || !PostProcessColor->CreateResources(Device)) {
            return false;
        }

        mColor = std::move(Color);
        mDepth = std::move(Depth);
        mSceneColor = std::move(SceneColor);
        mPostProcessColor = std::move(PostProcessColor);

        mWidth = Width;
        mHeight = Height;

        return true;
    }

    /**
     * @brief 가로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Renderer::FDXRenderTarget::GetWidth() const {
        return mWidth;
    }

    /**
     * @brief 세로 픽셀 수를 반환합니다.
     */
    Core::Uint32 Renderer::FDXRenderTarget::GetHeight() const {
        return mHeight;
    }

    /**
     * @brief UI에서 샘플링할 셰이더 리소스 뷰의 비소유 주소를 반환합니다.
     */
    void* Renderer::FDXRenderTarget::GetNativeTexture() const {
        return mColor != nullptr ? mColor->GetShaderResourceView() : nullptr;
    }

    /**
     * @brief 컬러 출력용 Direct3D 11 렌더 타깃 뷰의 비소유 주소를 반환합니다.
     */
    ID3D11RenderTargetView* Renderer::FDXRenderTarget::GetRenderTargetView() const {
        return mColor != nullptr ? mColor->GetRenderTargetView() : nullptr;
    }

    /**
     * @brief 깊이 스텐실 뷰를 반환합니다.
     */
    ID3D11DepthStencilView* Renderer::FDXRenderTarget::GetDepthView() const {
        return mDepth != nullptr ? mDepth->GetDepthStencilView() : nullptr;
    }

    std::size_t Renderer::FDXRenderTarget::GetResourceBytes() const {
        return (mColor != nullptr ? mColor->GetResourceBytes() : 0) + (mDepth != nullptr ? mDepth->GetResourceBytes() : 0) + (mSceneColor != nullptr ? mSceneColor->GetResourceBytes() : 0) + (mPostProcessColor != nullptr ? mPostProcessColor->GetResourceBytes() : 0);
    }

    Asset::UTexture2D* Renderer::FDXRenderTarget::GetSceneColor() const {
        return mSceneColor.get();
    }

    Asset::UTexture2D* Renderer::FDXRenderTarget::GetPostProcessColor() const {
        return mPostProcessColor.get();
    }

    Asset::UTexture2D* Renderer::FDXRenderTarget::GetOutputColor() const {
        return mColor.get();
    }

    Asset::UTexture2D* Renderer::FDXRenderTarget::GetSceneDepth() const {
        return mDepth.get();
    }

}
