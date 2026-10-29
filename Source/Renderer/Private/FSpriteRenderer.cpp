/**
 * @file FSpriteRenderer.cpp
 * @brief 텍스처 스프라이트와 기본 애니메이션 아틀라스의 GPU 자원을 관리합니다.
 * @ingroup Renderer
 */

#include "FSpriteRenderer.h"
#include "RendererStats.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FSceneView.h"
#include "Engine/SpriteGeometry.h"
#include "CoreUObject/Reflection/NewObject.h"

#include <cmath>

FSpriteRenderer::FSpriteRenderer() = default;
FSpriteRenderer::~FSpriteRenderer() = default;

bool FSpriteRenderer::Initialize(ID3D11Device* Device) {
    D3D11_BUFFER_DESC Buffer{};
    Buffer.ByteWidth = sizeof(FConstants);
    Buffer.Usage = D3D11_USAGE_DEFAULT;
    Buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SAMPLER_DESC Sampler{};
    Sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(Device->CreateBuffer(&Buffer, nullptr, mConstants.GetAddressOf())) || FAILED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf()))) {
        return false;
    }
    TArray<Uint8> Pixels{};
    Pixels.resize(128 * 128 * 4);
    for (Uint32 Y{}; Y < 128; ++Y) {
        for (Uint32 X{}; X < 128; ++X) {
            const Uint32 Frame{(Y / 32) * 4 + X / 32};
            const float Horizontal{(static_cast<float>(X % 32) + 0.5f) / 16.0f - 1.0f};
            const float Vertical{(static_cast<float>(Y % 32) + 0.5f) / 16.0f - 1.0f};
            const float Radius{0.25f + 0.6f * (static_cast<float>(Frame) / 15.0f)};
            const bool Visible{Horizontal * Horizontal + Vertical * Vertical <= Radius * Radius};
            const std::size_t Offset{(static_cast<std::size_t>(Y) * 128 + X) * 4};
            Pixels[Offset] = static_cast<Uint8>(64 + Frame * 12);
            Pixels[Offset + 1] = static_cast<Uint8>(220 - Frame * 8);
            Pixels[Offset + 2] = 240;
            Pixels[Offset + 3] = Visible ? 255 : 0;
        }
    }
    mDefaultAtlas = NewObject<UTexture2D>();
    return mDefaultAtlas->SetPixels(128, 128, ETextureFormat::RGBA8, std::move(Pixels)) && mDefaultAtlas->CreateResources(Device);
}

bool FSpriteRenderer::CreateShaders(ID3D11Device* Device, std::span<const Uint8> VertexCode, std::span<const Uint8> PixelCode) {
    Microsoft::WRL::ComPtr<ID3D11VertexShader> Vertex{};
    Microsoft::WRL::ComPtr<ID3D11PixelShader> Pixel{};
    if (FAILED(Device->CreateVertexShader(VertexCode.data(), VertexCode.size(), nullptr, Vertex.GetAddressOf())) || FAILED(Device->CreatePixelShader(PixelCode.data(), PixelCode.size(), nullptr, Pixel.GetAddressOf()))) {
        return false;
    }
    mVertexShader = std::move(Vertex);
    mPixelShader = std::move(Pixel);
    return true;
}

bool FSpriteRenderer::Draw(ID3D11DeviceContext* Context, const FPrimitiveSceneProxy& Proxy, const FSceneView& View) {
    if (!Proxy.mSprite.has_value() || mVertexShader == nullptr || mPixelShader == nullptr) {
        return false;
    }
    const FDrawSprite& Sprite{*Proxy.mSprite};
    UTexture2D* Texture{Sprite.mTexture != nullptr ? Sprite.mTexture : mDefaultAtlas.get()};
    Microsoft::WRL::ComPtr<ID3D11Device> Device{};
    Context->GetDevice(Device.GetAddressOf());
    if (Texture == nullptr || (!Texture->HasResources() && !Texture->CreateResources(Device.Get()))) {
        return false;
    }
    const FConstants Constants{MakeSpriteWorld(Proxy.mWorld, Sprite, View) * View.mViewProjection, {Proxy.mUVScale.x, Proxy.mUVScale.y, Proxy.mUVOffset.x, Proxy.mUVOffset.y}, Sprite.mColor};
    Context->UpdateSubresource(mConstants.Get(), 0, nullptr, &Constants, 0, 0);
    ID3D11Buffer* Buffer{mConstants.Get()};
    ID3D11ShaderResourceView* Resource{Texture->GetShaderResourceView()};
    ID3D11SamplerState* Sampler{mSampler.Get()};
    Context->IASetInputLayout(nullptr);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Context->VSSetShader(mVertexShader.Get(), nullptr, 0);
    Context->PSSetShader(mPixelShader.Get(), nullptr, 0);
    Context->VSSetConstantBuffers(0, 1, &Buffer);
    Context->PSSetConstantBuffers(0, 1, &Buffer);
    Context->PSSetShaderResources(0, 1, &Resource);
    Context->PSSetSamplers(0, 1, &Sampler);
    Context->Draw(6, 0);
    RendererStats::Triangles.Add(2.0);
    ID3D11ShaderResourceView* Empty{};
    Context->PSSetShaderResources(0, 1, &Empty);
    return true;
}

std::size_t FSpriteRenderer::GetResourceBytes() const {
    return sizeof(FConstants) + (mDefaultAtlas != nullptr ? mDefaultAtlas->GetResourceBytes() : 0);
}
