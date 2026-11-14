/**
 * @file FSkyResources.cpp
 * @brief 등장방형 파노라마를 전체 화면 삼각형으로 그리는 기본 하늘 패스의 자원입니다.
 * @ingroup Renderer
 */

#include "Core/Types.h"
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

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        // DX10 확장을 포함한 DDS 헤더를 32비트 단위로 읽을 때의 필드 위치입니다.

        /**
         * @brief DX10 확장까지 포함한 DDS 헤더 워드 수입니다.
         */
        constexpr std::size_t HeaderWords{37};

        /**
         * @brief DDS 매직 값의 워드 위치입니다.
         */
        constexpr Core::Index_t MagicWord{0};

        /**
         * @brief DDS 텍스처 높이의 워드 위치입니다.
         */
        constexpr Core::Index_t HeightWord{3};

        /**
         * @brief DDS 텍스처 너비의 워드 위치입니다.
         */
        constexpr Core::Index_t WidthWord{4};

        /**
         * @brief DDS 밉 개수의 워드 위치입니다.
         */
        constexpr Core::Index_t MipCountWord{7};

        /**
         * @brief DDS 기본 헤더 FourCC의 워드 위치입니다.
         */
        constexpr Core::Index_t FourCCWord{21};

        /**
         * @brief DDS DX10 DXGI 포맷의 워드 위치입니다.
         */
        constexpr Core::Index_t FormatWord{32};

        /**
         * @brief DDS DX10 리소스 차원의 워드 위치입니다.
         */
        constexpr Core::Index_t DimensionWord{33};

        /**
         * @brief DDS DX10 배열 크기의 워드 위치입니다.
         */
        constexpr Core::Index_t ArraySizeWord{35};

        /**
         * @brief DDS 파일 식별 매직 값입니다.
         */
        constexpr Core::Uint32 DDSMagic{0x20534444u};

        /**
         * @brief DX10 확장 DDS 헤더의 FourCC입니다.
         */
        constexpr Core::Uint32 DX10FourCC{0x30315844u};

        /**
         * @brief DDS DX10의 2차원 텍스처 차원 값입니다.
         */
        constexpr Core::Uint32 Texture2DDimension{3u};

        /**
         * @brief BC7 블록 하나의 바이트 수입니다.
         */
        constexpr std::size_t BC7BlockBytes{16};
    }

    /**
     * @brief 기본 설정으로 FSkyResources 객체를 초기화합니다.
     */
    Renderer::FSkyResources::FSkyResources() = default;

    /**
     * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Renderer::FSkyResources::~FSkyResources() = default;

    /**
     * @brief 상수 버퍼·샘플러·깊이·래스터라이저 상태를 생성하고 파노라마를 불러옵니다.
     */
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

    /**
     * @brief 정점·픽셀 바이트 코드로 하늘 셰이더를 생성하고 둘 다 성공한 경우에만 교체합니다.
     */
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

    /**
     * @brief 불투명 패스의 깊이가 비어 있는 픽셀에만 하늘을 그립니다.
     */
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

    /**
     * @brief BC7 DDS를 CPU 변환 없이 밉 체인 그대로 GPU 텍스처로 생성합니다.
     */
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

        for (Core::Index_t Mip{}; Mip < MipCount; ++Mip) {
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
