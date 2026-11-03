#include "FSkyResources.h"
#include "FPassTarget.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Math/FMatrix.h"
#include "Engine/FSceneView.h"

#include <dxgiformat.h>
#include <algorithm>
#include <array>
#include <fstream>

namespace Renderer {

    namespace {
        // DX10 확장을 포함한 DDS 헤더를 32비트 단위로 읽을 때의 필드 위치입니다.
        constexpr std::size_t HeaderWords{37};
        constexpr std::size_t MagicWord{0};
        constexpr std::size_t HeightWord{3};
        constexpr std::size_t WidthWord{4};
        constexpr std::size_t MipCountWord{7};
        constexpr std::size_t FourCCWord{21};
        constexpr std::size_t FormatWord{32};
        constexpr std::size_t DimensionWord{33};
        constexpr std::size_t ArraySizeWord{35};

        constexpr Core::Uint32 DDSMagic{0x20534444u};
        constexpr Core::Uint32 DX10FourCC{0x30315844u};
        constexpr Core::Uint32 Texture2DDimension{3u};
        constexpr std::size_t BC7BlockBytes{16};
    }

    Renderer::FSkyResources::FSkyResources() = default;
    Renderer::FSkyResources::~FSkyResources() = default;

    bool Renderer::FSkyResources::Initialize(ID3D11Device* Device, const std::filesystem::path& PanoramaPath) {
        D3D11_SAMPLER_DESC Sampler{};
        Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Sampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.MaxLOD = D3D11_FLOAT32_MAX;

        D3D11_DEPTH_STENCIL_DESC Depth{};
        Depth.DepthEnable = true;
        Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

        D3D11_RASTERIZER_DESC Rasterizer{};
        Rasterizer.FillMode = D3D11_FILL_SOLID;
        Rasterizer.CullMode = D3D11_CULL_NONE;

        Core::FString Error{};

        if (!mConstants.Create(Device, Error) || FAILED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf())) || FAILED(Device->CreateDepthStencilState(&Depth, mDepthState.GetAddressOf())) || FAILED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf()))) {
            return false;
        }

        if (!LoadPanorama(Device, PanoramaPath)) {
            Core::ErrorHandler::Report("Sky", "Cannot load the sky panorama: " + PanoramaPath.string(), Core::ErrorHandler::EErrorLevel::Warning);
        }

        return true;
    }

    bool Renderer::FSkyResources::CreateShaders(ID3D11Device* Device, std::span<const Core::Uint8> VertexCode, std::span<const Core::Uint8> PixelCode) {
        Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
        Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};

        if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
            return false;
        }

        mVertexShader = std::move(Vertex);
        mPixelShader = std::move(Pixel);

        return true;
    }

    Core::Uint32 Renderer::FSkyResources::Render(ID3D11DeviceContext* Context, const Renderer::FPassTarget& Target, const Engine::FSceneView& View) {
        if (mPanorama == nullptr || mVertexShader == nullptr || mPixelShader == nullptr) {
            return 0;
        }

        // 파노라마는 방향만 필요하므로 이동을 빼서 카메라가 멀리 있어도 정밀도를 유지합니다.
        Core::FMatrix ViewRotation{View.mView};
        ViewRotation(3, 0) = 0.0f;
        ViewRotation(3, 1) = 0.0f;
        ViewRotation(3, 2) = 0.0f;

        Core::FMatrix InverseViewRotationProjection{};

        if (!(ViewRotation * View.mProjection).TryInverse(InverseViewRotationProjection)) {
            return 0;
        }

        mConstants.Update(Context, InverseViewRotationProjection);
        Context->OMSetRenderTargets(1, &Target.mColor, Target.mDepth);
        Context->OMSetDepthStencilState(mDepthState.Get(), 0);
        Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        Context->RSSetViewports(1, &Target.mViewport);
        Context->RSSetState(mRasterizer.Get());
        Context->IASetInputLayout(nullptr);
        Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
        Context->PSSetShader(mPixelShader.Get(), nullptr, 0);

        ID3D11SamplerState* Sampler{mSampler.Get()};
        ID3D11ShaderResourceView* Panorama{mPanorama.Get()};
        mConstants.Bind(Context, RenderCore::EShaderStage::Pixel, 0);
        Context->PSSetSamplers(0, 1, &Sampler);
        Context->PSSetShaderResources(0, 1, &Panorama);
        Context->Draw(3, 0);

        ID3D11ShaderResourceView* Empty{};
        Context->PSSetShaderResources(0, 1, &Empty);

        return 1;
    }

    bool Renderer::FSkyResources::LoadPanorama(ID3D11Device* Device, const std::filesystem::path& Path) {
        std::error_code Code{};
        const auto Size{std::filesystem::file_size(Path, Code)};
        std::ifstream Input{Path, std::ios::binary};
        std::array<Core::Uint32, HeaderWords> Header{};

        if (Code || Size < sizeof(Header) || !Input || !Input.read(reinterpret_cast<char*>(Header.data()), sizeof(Header))) {
            return false;
        }

        const bool BC7{Header[FormatWord] == DXGI_FORMAT_BC7_UNORM || Header[FormatWord] == DXGI_FORMAT_BC7_UNORM_SRGB};

        if (Header[MagicWord] != DDSMagic || Header[FourCCWord] != DX10FourCC || !BC7 || Header[DimensionWord] != Texture2DDimension || Header[ArraySizeWord] != 1 || Header[WidthWord] == 0 || Header[HeightWord] == 0 || Header[WidthWord] > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Header[HeightWord] > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
            return false;
        }

        const Core::Uint32 MipCount{std::max(Header[MipCountWord], 1u)};

        if (MipCount > D3D11_REQ_MIP_LEVELS) {
            return false;
        }

        Core::TArray<Core::Uint8> Pixels(static_cast<std::size_t>(Size) - sizeof(Header));

        if (!Input.read(reinterpret_cast<char*>(Pixels.data()), static_cast<std::streamsize>(Pixels.size()))) {
            return false;
        }

        Core::TArray<D3D11_SUBRESOURCE_DATA> InitialData{};
        InitialData.reserve(MipCount);
        std::size_t Offset{};

        for (Core::Uint32 Mip{}; Mip < MipCount; ++Mip) {
            const std::size_t BlocksWide{(static_cast<std::size_t>(std::max(Header[WidthWord] >> Mip, 1u)) + 3) / 4};
            const std::size_t BlocksHigh{(static_cast<std::size_t>(std::max(Header[HeightWord] >> Mip, 1u)) + 3) / 4};
            const std::size_t RowPitch{BlocksWide * BC7BlockBytes};
            const std::size_t SlicePitch{RowPitch * BlocksHigh};

            if (SlicePitch > Pixels.size() - Offset) {
                return false;
            }

            InitialData.push_back({Pixels.data() + Offset, static_cast<UINT>(RowPitch), static_cast<UINT>(SlicePitch)});
            Offset += SlicePitch;
        }

        D3D11_TEXTURE2D_DESC Description{};
        Description.Width = Header[WidthWord];
        Description.Height = Header[HeightWord];
        Description.MipLevels = MipCount;
        Description.ArraySize = 1;
        // 장면 컬러가 표시 공간 값을 그대로 담으므로 sRGB 디코드 없이 읽습니다.
        Description.Format = DXGI_FORMAT_BC7_UNORM;
        Description.SampleDesc.Count = 1;
        Description.Usage = D3D11_USAGE_IMMUTABLE;
        Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture{};

        return SUCCEEDED(Device->CreateTexture2D(&Description, InitialData.data(), Texture.GetAddressOf())) && SUCCEEDED(Device->CreateShaderResourceView(Texture.Get(), nullptr, mPanorama.ReleaseAndGetAddressOf()));
    }

}
