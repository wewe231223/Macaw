/**
 * @file FMaterialPipeline.cpp
 * @brief 머티리얼 셰이더와 양면 정책별 GPU 파이프라인을 관리합니다.
 * @ingroup Renderer
 */

#include "FMaterialPipeline.h"

#include <array>
#include <utility>

using namespace Microsoft::WRL;

FMaterialPipeline::FMaterialPipeline(std::shared_ptr<const FShader> VertexCode, std::shared_ptr<const FShader> PixelCode, bool TwoSided)
	: mVertexCode{std::move(VertexCode)},
	  mPixelCode{std::move(PixelCode)},
	  mTwoSided{TwoSided} {
}

FMaterialPipeline::~FMaterialPipeline() = default;

bool FMaterialPipeline::IsCurrent() const {
    return HasResources() && mVertexRevision == mVertexCode->GetRevision() && mPixelRevision == mPixelCode->GetRevision();
}

bool FMaterialPipeline::HasResources() const {
    return mVertexShader != nullptr && mPixelShader != nullptr && mInputLayout != nullptr && mRasterizer != nullptr;
}

bool FMaterialPipeline::CreateResources(ID3D11Device* Device, FString& Error) {
    Error.clear();
    if (Device == nullptr || mVertexCode == nullptr || mPixelCode == nullptr) {
        Error = "Invalid material shader";
        return false;
    }
    const auto VertexCompilation{mVertexCode->GetCompilation()};
    const auto PixelCompilation{mPixelCode->GetCompilation()};
    const std::span<const Uint8> VertexBytes{VertexCompilation->mByteCode};
    const std::span<const Uint8> PixelBytes{PixelCompilation->mByteCode};
    const std::array<D3D11_INPUT_ELEMENT_DESC, 5> Elements{{{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}, {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}}};
    ComPtr<ID3D11VertexShader> Vertex{};
    ComPtr<ID3D11PixelShader> Pixel{};
    ComPtr<ID3D11InputLayout> Layout{};
    ComPtr<ID3D11RasterizerState> Rasterizer{};
    D3D11_RASTERIZER_DESC Description{};
    Description.FillMode = D3D11_FILL_SOLID;
    Description.CullMode = mTwoSided ? D3D11_CULL_NONE : D3D11_CULL_BACK;
    Description.DepthClipEnable = true;
    if (FAILED(Device->CreateVertexShader(VertexBytes.data(), VertexBytes.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelBytes.data(), PixelBytes.size(), nullptr, Pixel.GetAddressOf())) || FAILED(Device->CreateInputLayout(Elements.data(), static_cast<UINT>(Elements.size()), VertexBytes.data(), VertexBytes.size(), Layout.GetAddressOf())) || FAILED(Device->CreateRasterizerState(&Description, Rasterizer.GetAddressOf()))) {
        Error = "Cannot create the material pipeline";
        return false;
    }
    mVertexShader = std::move(Vertex);
    mPixelShader = std::move(Pixel);
    mInputLayout = std::move(Layout);
    mRasterizer = std::move(Rasterizer);
    mVertexRevision = mVertexCode->GetRevision();
    mPixelRevision = mPixelCode->GetRevision();
    return true;
}

void FMaterialPipeline::Bind(ID3D11DeviceContext* Context) const {
    /**
     * @brief 사용할 정점 셰이더의 비소유 주소입니다.
     */
    ID3D11VertexShader* VertexShader{mVertexShader.Get()};
    /**
     * @brief 사용할 픽셀 셰이더의 비소유 주소입니다.
     */
    ID3D11PixelShader* PixelShader{mPixelShader.Get()};
    /**
     * @brief 사용할 정점 입력 레이아웃의 비소유 주소입니다.
     */
    ID3D11InputLayout* InputLayout{mInputLayout.Get()};
    Context->VSSetShader(VertexShader, nullptr, 0);
    Context->PSSetShader(PixelShader, nullptr, 0);
    Context->IASetInputLayout(InputLayout);
    Context->RSSetState(mRasterizer.Get());
}
