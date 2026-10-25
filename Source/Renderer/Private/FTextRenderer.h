#pragma once

#include "FFontAtlas.h"
#include "Engine/UTexture2D.h"
#include "Core/Math/FMatrix.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <span>

struct FDrawText;
struct FSceneView;

class FTextRenderer {
public:
    ReflectTypes(FTextRenderer);

private:
    struct FTextConstants {
    public:
        ReflectTypes(FTextConstants);

    public:
        ReflectStruct(FTextConstants);

    private:
        ReflectMembers();

    public:
        FMatrix mTransform{};
        ReflectProperty(mTransform, "Transform", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        FVector4 mColor{};
        ReflectProperty(mColor, "Color", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
        FVector4 mParameters{};
        ReflectProperty(mParameters, "Parameters", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    };

public:
    ReflectStruct(FTextRenderer);

    bool Initialize(ID3D11Device* Device, const std::filesystem::path& FontPath);
    bool CreateShaders(ID3D11Device* Device, std::span<const Uint8> VertexCode, std::span<const Uint8> PixelCode);
    bool Draw(ID3D11DeviceContext* Context, const FDrawText& Text, const FSceneView& View);
    std::size_t GetResourceBytes() const;
    bool GetBounds(const FString& Text, FVector2& Minimum, FVector2& Maximum) const;

private:
    ReflectMembers();

private:
    FFontAtlas mAtlas{};
    ReflectProperty(mAtlas, "Atlas", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::unique_ptr<UTexture2D> mTexture{};
    ReflectProperty(mTexture, "Texture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader{};
    ReflectProperty(mVertexShader, "VertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader{};
    ReflectProperty(mPixelShader, "PixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLayout{};
    ReflectProperty(mLayout, "Layout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    ReflectProperty(mSampler, "Sampler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstants{};
    ReflectProperty(mConstants, "Constants", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mVertices{};
    ReflectProperty(mVertices, "Vertices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mCapacity{};
    ReflectProperty(mCapacity, "Capacity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TMap<FString, TArray<FTextVertex>> mLayouts{};
    ReflectProperty(mLayouts, "Layouts", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
