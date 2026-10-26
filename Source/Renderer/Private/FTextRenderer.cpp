#include "FTextRenderer.h"
#include "Engine/FDrawText.h"
#include "Engine/FSceneView.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Microsoft::WRL;

bool FTextRenderer::Initialize(ID3D11Device* Device, const std::filesystem::path& FontPath) {
    auto Texture{NewObject<UTexture2D>()};
    std::filesystem::path ImagePath{FontPath};
    ImagePath.replace_extension(L".png");
    if (!mAtlas.Load(FontPath) || !Texture->LoadFromFile(ImagePath, false) || Texture->GetSizeX() != mAtlas.GetWidth() || Texture->GetSizeY() != mAtlas.GetHeight() || !Texture->CreateResources(Device)) {
        ErrorHandler::Report("Text", "Cannot load the font atlas", ErrorHandler::EErrorLevel::Error);
        return false;
    }

    D3D11_BUFFER_DESC Constants{};
    Constants.ByteWidth = sizeof(FTextConstants);
    Constants.Usage = D3D11_USAGE_DEFAULT;
    Constants.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SAMPLER_DESC Sampler{};
    Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(Device->CreateBuffer(&Constants, nullptr, mConstants.GetAddressOf())) || FAILED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf()))) {
        return false;
    }
    mTexture = std::move(Texture);
    return true;
}

bool FTextRenderer::CreateShaders(ID3D11Device* Device, std::span<const Uint8> VertexCode, std::span<const Uint8> PixelCode) {
    ComPtr<ID3D11VertexShader> Vertex{};
    ComPtr<ID3D11PixelShader> Pixel{};
    ComPtr<ID3D11InputLayout> Layout{};
    const std::array<D3D11_INPUT_ELEMENT_DESC, 2> Elements{{{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FTextVertex, mPosition), D3D11_INPUT_PER_VERTEX_DATA, 0}, {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(FTextVertex, mUV), D3D11_INPUT_PER_VERTEX_DATA, 0}}};
    if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf())) || FAILED(Device->CreateInputLayout(Elements.data(), static_cast<UINT>(Elements.size()), VertexCode.data(), VertexCode.size(), Layout.GetAddressOf()))) {
        return false;
    }
    mVertexShader = std::move(Vertex);
    mPixelShader = std::move(Pixel);
    mLayout = std::move(Layout);
    return true;
}

bool FTextRenderer::Draw(ID3D11DeviceContext* Context, const FDrawText& Text, const FSceneView& View) {
    if (Text.mText.empty() || Text.mText.size() > 65536 || !std::isfinite(Text.mSize) || Text.mSize <= 0.0f || !Text.mPosition.IsFinite() || !Text.mScale.IsFinite() || !Text.mColor.IsFinite() || Text.mColor.w <= 0.0f || View.mSize.x <= 0.0f || View.mSize.y <= 0.0f) {
        return false;
    }
    const FVector4 Clip{Text.mScreenSpace ? FVector4{2.0f * Text.mPosition.x / View.mSize.x - 1.0f, 1.0f - 2.0f * Text.mPosition.y / View.mSize.y, Text.mPosition.z, 1.0f} : FVector4{Text.mPosition, 1.0f} * View.mViewProjection};
    if (Clip.w <= 0.0001f) {
        return false;
    }

    FTextConstants Constants{};
    if (Text.mScreenSpace) {
        if (Clip.z < 0.0f || Clip.z > Clip.w) {
            return false;
        }
        Constants.mTransform(0, 0) = 2.0f * Text.mSize * Text.mScale.x * Clip.w / View.mSize.x;
        Constants.mTransform(1, 1) = 2.0f * Text.mSize * Text.mScale.y * Clip.w / View.mSize.y;
        Constants.mTransform(3, 0) = Clip.x;
        Constants.mTransform(3, 1) = Clip.y;
        Constants.mTransform(3, 2) = Clip.z;
        Constants.mTransform(3, 3) = Clip.w;
    } else {
        FMatrix World{};
        for (Int32 Column{}; Column < 3; ++Column) {
            World(0, Column) = View.mView(Column, 0) * Text.mSize * Text.mScale.x;
            World(1, Column) = View.mView(Column, 1) * Text.mSize * Text.mScale.y;
        }
        World(3, 0) = Text.mPosition.x;
        World(3, 1) = Text.mPosition.y;
        World(3, 2) = Text.mPosition.z;
        Constants.mTransform = World * View.mViewProjection;
    }
    Constants.mColor = Text.mColor;
    Constants.mParameters = {mAtlas.GetDistanceRange(), static_cast<float>(mAtlas.GetWidth()), static_cast<float>(mAtlas.GetHeight()), 0.0f};

    if (!mLayouts.contains(Text.mText)) {
        if (mLayouts.size() >= 128) {
            mLayouts.clear();
        }
        mLayouts.emplace(Text.mText, mAtlas.BuildVertices(Text.mText));
    }
    const auto& Vertices{mLayouts.at(Text.mText)};
    if (Vertices.empty()) {
        return false;
    }
    const Uint32 Count{static_cast<Uint32>(Vertices.size())};
    if (Count > mCapacity) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = std::max(Count, 256u) * sizeof(FTextVertex);
        Description.Usage = D3D11_USAGE_DYNAMIC;
        Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ComPtr<ID3D11Device> Device{};
        Context->GetDevice(Device.GetAddressOf());
        ComPtr<ID3D11Buffer> Buffer{};
        if (FAILED(Device->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf()))) {
            ErrorHandler::Report("Text", "Text vertex buffer allocation failed", ErrorHandler::EErrorLevel::Error);
            return false;
        }
        mVertices = std::move(Buffer);
        mCapacity = std::max(Count, 256u);
    }
    D3D11_MAPPED_SUBRESOURCE Mapped{};
    if (FAILED(Context->Map(mVertices.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) {
        return false;
    }
    std::memcpy(Mapped.pData, Vertices.data(), Vertices.size() * sizeof(FTextVertex));
    Context->Unmap(mVertices.Get(), 0);
    Context->UpdateSubresource(mConstants.Get(), 0, nullptr, &Constants, 0, 0);

    ID3D11Buffer* VertexBuffer{mVertices.Get()};
    ID3D11Buffer* ConstantBuffer{mConstants.Get()};
    ID3D11ShaderResourceView* Atlas{mTexture->GetShaderResourceView()};
    ID3D11SamplerState* Sampler{mSampler.Get()};
    const UINT Stride{sizeof(FTextVertex)};
    const UINT Offset{};
    Context->IASetInputLayout(mLayout.Get());
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
    Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
    Context->VSSetConstantBuffers(0, 1, &ConstantBuffer);
    Context->PSSetConstantBuffers(0, 1, &ConstantBuffer);
    Context->PSSetShaderResources(0, 1, &Atlas);
    Context->PSSetSamplers(0, 1, &Sampler);
    Context->Draw(Count, 0);
    ID3D11ShaderResourceView* Empty{};
    Context->PSSetShaderResources(0, 1, &Empty);
    return true;
}

std::size_t FTextRenderer::GetResourceBytes() const {
    return (mTexture != nullptr ? mTexture->GetResourceBytes() : 0) + mCapacity * sizeof(FTextVertex) + sizeof(FTextConstants);
}

bool FTextRenderer::GetBounds(const FString& Text, FVector2& Minimum, FVector2& Maximum) const {
    if (Text.size() > 65536) {
        return false;
    }
    const TArray<FTextVertex> Vertices{mAtlas.BuildVertices(Text)};
    if (Vertices.empty()) {
        return false;
    }
    Minimum = Vertices.front().mPosition;
    Maximum = Minimum;
    for (const FTextVertex& Vertex : Vertices) {
        Minimum = {std::min(Minimum.x, Vertex.mPosition.x), std::min(Minimum.y, Vertex.mPosition.y)};
        Maximum = {std::max(Maximum.x, Vertex.mPosition.x), std::max(Maximum.y, Vertex.mPosition.y)};
    }
    return true;
}
