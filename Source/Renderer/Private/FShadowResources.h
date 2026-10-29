#pragma once

#include "Core/ReflectionMacros.h"

#include "FShadowAtlasAllocator.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneViewFamily.h"
#include "Engine/UTexture2D.h"
#include "Engine/UStaticMesh.h"
#include "Engine/EPrimitiveType.h"
#include "Core/Math/FBox.h"
#include "RenderCore/FRenderStats.h"
#include "RenderCore/FShadowAtlasView.h"

#include <span>

class FScene;

struct FGpuShadow {
public:
    ReflectTypes(FGpuShadow);

public:
    ReflectStruct(FGpuShadow);

private:
    ReflectMembers();

public:
    FMatrix mViewProjection{};
    ReflectProperty(mViewProjection, "ViewProjection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mAtlasRectangle{};
    ReflectProperty(mAtlasRectangle, "AtlasRectangle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mParameters{};
    ReflectProperty(mParameters, "Parameters", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FVector4 mCascade{};
    ReflectProperty(mCascade, "Cascade", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

class FShadowResources {
public:
    ReflectTypes(FShadowResources);

public:
    ReflectStruct(FShadowResources);

public:
    bool Render(ID3D11Device* Device, ID3D11DeviceContext* Context, const FScene& Scene, const FSceneViewFamily& Family, const std::array<std::unique_ptr<UStaticMesh>, static_cast<std::size_t>(EPrimitiveType::Count)>& Meshes, FRenderStats& Stats);
    void Bind(ID3D11DeviceContext* Context) const;
    void Unbind(ID3D11DeviceContext* Context) const;
    const TArray<std::array<Uint32, 2>>& GetLightRanges() const;
    FShadowAtlasView GetView() const;
    static FMatrix MakeDirectionalMatrix(const FSceneView& View, const FVector& Direction, float NearDistance, float FarDistance, Uint32 Resolution, std::span<const FBox> CasterBounds);

private:
    ReflectMembers();

    bool CreateResources(ID3D11Device* Device);
    void Build(const FScene& Scene, const FSceneViewFamily& Family, FRenderStats& Stats);
    bool Upload(ID3D11Device* Device, ID3D11DeviceContext* Context);
    void RenderPreview(ID3D11DeviceContext* Context);

private:
    std::unique_ptr<UTexture2D> mAtlas{};
    ReflectProperty(mAtlas, "Atlas", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::unique_ptr<UTexture2D> mPreview{};
    ReflectProperty(mPreview, "Preview", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    FShadowAtlasAllocator mAllocator{};
    ReflectProperty(mAllocator, "Allocator", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FGpuShadow> mMaps{};
    ReflectProperty(mMaps, "Maps", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<FShadowAtlasTile> mTiles{};
    ReflectProperty(mTiles, "Tiles", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    TArray<std::array<Uint32, 2>> mLightRanges{};
    ReflectProperty(mLightRanges, "LightRanges", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mDepthShader{};
    ReflectProperty(mDepthShader, "DepthShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11VertexShader> mPreviewVertexShader{};
    ReflectProperty(mPreviewVertexShader, "PreviewVertexShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11PixelShader> mPreviewPixelShader{};
    ReflectProperty(mPreviewPixelShader, "PreviewPixelShader", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11InputLayout> mLayout{};
    ReflectProperty(mLayout, "Layout", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mMapBuffer{};
    ReflectProperty(mMapBuffer, "MapBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mShadowBuffer{};
    ReflectProperty(mShadowBuffer, "ShadowBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mViewBuffer{};
    ReflectProperty(mViewBuffer, "ViewBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShadowView{};
    ReflectProperty(mShadowView, "ShadowView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11SamplerState> mSampler{};
    ReflectProperty(mSampler, "Sampler", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> mRasterizer{};
    ReflectProperty(mRasterizer, "Rasterizer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mDepthState{};
    ReflectProperty(mDepthState, "DepthState", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> mPreviewDepthState{};
    ReflectProperty(mPreviewDepthState, "PreviewDepthState", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mCapacity{};
    ReflectProperty(mCapacity, "Capacity", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mEnabled{};
    ReflectProperty(mEnabled, "Enabled", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
