/**
 * @file FShadowResources.cpp
 * @brief 광원별 그림자 맵을 아틀라스에 렌더링하고 셰이더·미리보기 자원을 관리합니다.
 * @ingroup Renderer
 */

#include "FShadowResources.h"

#include "FScene.h"
#include "FScenePrimitive.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/UStaticMesh.h"

#include <d3dcompiler.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <numeric>

using namespace Microsoft::WRL;

/**
 * @brief 그림자 내장 셰이더와 아틀라스 구성에 사용하는 내부 도우미입니다.
 */
namespace {
    /**
     * @brief Direct3D 11에서 지원하는 그림자 아틀라스 한 변의 최대 픽셀 수입니다.
     */
    constexpr Uint32 AtlasSize{D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION};
    /**
     * @brief 그림자 타일의 각 가장자리에 확보하는 보호 픽셀 수입니다.
     */
    constexpr Uint32 Guard{4};
    /**
     * @brief 그림자 깊이와 아틀라스 미리보기 패스의 내장 HLSL 소스입니다.
     */
    constexpr char ShadowShader[]{"cbuffer ObjectConstants : register(b0) { row_major float4x4 World; };" "cbuffer ShadowMatrix : register(b1) { row_major float4x4 ViewProjection; };" "float4 DepthMain(float3 Position : POSITION) : SV_POSITION { return mul(mul(float4(Position, 1), World), ViewProjection); }" "struct FOutput { float4 mPosition : SV_POSITION; float2 mUV : TEXCOORD0; };" "FOutput PreviewVertex(uint Index : SV_VertexID) { FOutput Output = {(FOutput)0}; Output.mUV = float2((Index << 1) & 2, Index & 2); Output.mPosition = float4(Output.mUV * float2(2, -2) + float2(-1, 1), 0, 1); return Output; }" "Texture2D<float> Atlas : register(t5);" "float4 PreviewPixel(FOutput Input) : SV_TARGET { uint Width = {0}; uint Height = {0}; Atlas.GetDimensions(Width, Height); float Depth = {Atlas.Load(int3(min(uint2(Input.mUV * float2(Width, Height)), uint2(Width - 1, Height - 1)), 0))}; float Value = {pow(saturate(Depth), 32)}; return float4(Value, Value, Value, 1); }"};

    /**
     * @brief 내장 그림자 HLSL의 지정한 진입점을 컴파일합니다.
     * @param[in] Entry 컴파일할 HLSL 진입점 이름입니다.
     * @param[in] Profile 컴파일 대상 셰이더 프로필입니다.
     * @param[out] Code 생성한 셰이더 바이트코드를 받을 COM 포인터입니다.
     * @return 셰이더 바이트코드 생성에 성공하면 true입니다.
     */
    bool CompileShadow(const char* Entry, const char* Profile, ComPtr<ID3DBlob>& Code) {
        ComPtr<ID3DBlob> Errors{};
        return SUCCEEDED(D3DCompile(ShadowShader, sizeof(ShadowShader) - 1, "ShadowAtlas", nullptr, nullptr, Entry, Profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, Code.GetAddressOf(), Errors.GetAddressOf()));
    }

    /**
     * @brief 광원 방향과 평행하지 않은 뷰 행렬용 위쪽 축을 선택합니다.
     * @param[in] Direction 광원이 비추는 월드 공간 방향입니다.
     * @return 안정적인 LookTo() 계산에 사용할 월드 Y축 또는 Z축입니다.
     */
    FVector LightUp(const FVector& Direction) {
        return std::abs(Direction.z) > 0.95f ? FVector{0.0f, 1.0f, 0.0f} : FVector{0.0f, 0.0f, 1.0f};
    }
}

static_assert(sizeof(FGpuShadow) == 112);

FMatrix FShadowResources::MakeDirectionalMatrix(const FSceneView& View, const FVector& Direction, float NearDistance, float FarDistance, Uint32 Resolution, std::span<const FBox> CasterBounds) {
    const FVector Forward{View.mForward.Normalized()};
    const FVector Center{View.mLocation + Forward * ((NearDistance + FarDistance) * 0.5f)};
    const float FarX{(View.mOrthographic ? 1.0f : FarDistance) / View.mProjection(0, 0)};
    const float FarY{(View.mOrthographic ? 1.0f : FarDistance) / View.mProjection(1, 1)};
    const float HalfDepth{(FarDistance - NearDistance) * 0.5f};
    const float Radius{std::ceil(std::sqrt(FarX * FarX + FarY * FarY + HalfDepth * HalfDepth) * 16.0f) / 16.0f * static_cast<float>(Resolution) / static_cast<float>(Resolution - 2)};
    const FMatrix Rotation{FMatrix::LookTo({}, Direction, LightUp(Direction))};
    const FVector4 LightCenter{FVector4{Center, 1.0f} * Rotation};
    const float Texel{2.0f * Radius / static_cast<float>(Resolution)};
    const float CenterX{std::round(LightCenter.x / Texel) * Texel};
    const float CenterY{std::round(LightCenter.y / Texel) * Texel};
    float MinimumZ{LightCenter.z - Radius};
    float MaximumZ{LightCenter.z + Radius};
    for (const FBox& Bounds : CasterBounds) {
        for (Uint32 Corner{}; Corner < 8; ++Corner) {
            const FVector Point{(Corner & 1) ? Bounds.mMaximum.x : Bounds.mMinimum.x, (Corner & 2) ? Bounds.mMaximum.y : Bounds.mMinimum.y, (Corner & 4) ? Bounds.mMaximum.z : Bounds.mMinimum.z};
            const FVector4 LightPoint{FVector4{Point, 1.0f} * Rotation};
            MinimumZ = std::min(MinimumZ, LightPoint.z - 0.1f);
            MaximumZ = std::max(MaximumZ, LightPoint.z + 0.1f);
        }
    }
    FMatrix Projection{FMatrix::Orthographic(2.0f * Radius, 2.0f * Radius, MinimumZ, MaximumZ)};
    Projection(3, 0) = -CenterX / Radius;
    Projection(3, 1) = -CenterY / Radius;
    return Rotation * Projection;
}

bool FShadowResources::CreateResources(ID3D11Device* Device) {
    if (mAtlas != nullptr && mDepthShader != nullptr && mPreview != nullptr && mSampler != nullptr && mMapBuffer != nullptr && mViewBuffer != nullptr && mLayout != nullptr && mDepthState != nullptr && mPreviewDepthState != nullptr && mRasterizer != nullptr && mPreviewVertexShader != nullptr && mPreviewPixelShader != nullptr) {
        return true;
    }
    auto Atlas{NewObject<UTexture2D>()};
    auto Preview{NewObject<UTexture2D>()};
    if (!Atlas->Configure(AtlasSize, AtlasSize, ETextureFormat::D24S8, ETextureUsage::DepthStencil | ETextureUsage::ShaderResource) || !Atlas->CreateResources(Device) || !Preview->Configure(2048, 2048, ETextureFormat::RGBA8, ETextureUsage::RenderTarget | ETextureUsage::ShaderResource) || !Preview->CreateResources(Device)) {
        return false;
    }
    ComPtr<ID3DBlob> DepthCode{};
    ComPtr<ID3DBlob> PreviewVertexCode{};
    ComPtr<ID3DBlob> PreviewPixelCode{};
    if (!CompileShadow("DepthMain", "vs_5_0", DepthCode) || !CompileShadow("PreviewVertex", "vs_5_0", PreviewVertexCode) || !CompileShadow("PreviewPixel", "ps_5_0", PreviewPixelCode)) {
        return false;
    }
    mDepthShader.Reset();
    mPreviewVertexShader.Reset();
    mPreviewPixelShader.Reset();
    mLayout.Reset();
    mSampler.Reset();
    mRasterizer.Reset();
    mDepthState.Reset();
    mPreviewDepthState.Reset();
    mMapBuffer.Reset();
    mViewBuffer.Reset();
    const D3D11_INPUT_ELEMENT_DESC Element{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
    D3D11_SAMPLER_DESC Sampler{};
    Sampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    Sampler.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
    Sampler.MaxLOD = D3D11_FLOAT32_MAX;
    D3D11_RASTERIZER_DESC Rasterizer{};
    Rasterizer.FillMode = D3D11_FILL_SOLID;
    Rasterizer.CullMode = D3D11_CULL_NONE;
    Rasterizer.DepthClipEnable = true;
    Rasterizer.ScissorEnable = true;
    D3D11_DEPTH_STENCIL_DESC Depth{};
    Depth.DepthEnable = true;
    Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    Depth.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    D3D11_BUFFER_DESC Buffer{};
    Buffer.ByteWidth = sizeof(FMatrix);
    Buffer.Usage = D3D11_USAGE_DEFAULT;
    Buffer.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(Device->CreateVertexShader(DepthCode->GetBufferPointer(), DepthCode->GetBufferSize(), nullptr, mDepthShader.GetAddressOf())) || FAILED(Device->CreateInputLayout(&Element, 1, DepthCode->GetBufferPointer(), DepthCode->GetBufferSize(), mLayout.GetAddressOf())) || FAILED(Device->CreateVertexShader(PreviewVertexCode->GetBufferPointer(), PreviewVertexCode->GetBufferSize(), nullptr, mPreviewVertexShader.GetAddressOf())) || FAILED(Device->CreatePixelShader(PreviewPixelCode->GetBufferPointer(), PreviewPixelCode->GetBufferSize(), nullptr, mPreviewPixelShader.GetAddressOf())) || FAILED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf())) || FAILED(Device->CreateRasterizerState(&Rasterizer, mRasterizer.GetAddressOf())) || FAILED(Device->CreateDepthStencilState(&Depth, mDepthState.GetAddressOf())) || FAILED(Device->CreateBuffer(&Buffer, nullptr, mMapBuffer.GetAddressOf()))) {
        return false;
    }
    Buffer.ByteWidth = sizeof(FVector4) * 2;
    Depth.DepthEnable = false;
    Depth.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    if (FAILED(Device->CreateBuffer(&Buffer, nullptr, mViewBuffer.GetAddressOf())) || FAILED(Device->CreateDepthStencilState(&Depth, mPreviewDepthState.GetAddressOf()))) {
        return false;
    }
    mAtlas = std::move(Atlas);
    mPreview = std::move(Preview);
    return true;
}

void FShadowResources::Build(const FScene& Scene, const FSceneViewFamily& Family, FRenderStats& Stats) {
    mAllocator.Reset();
    mMaps.clear();
    mTiles.clear();
    mLightRanges.assign(Scene.GetLights().size(), {});
    TArray<FBox> CasterBounds{};
    for (const FScenePrimitive& Primitive : Scene.GetPrimitives()) {
        if (Primitive.mProxy.mCastShadow && !Primitive.mProxy.mSprite.has_value() && !Primitive.mProxy.mText.has_value()) {
            CasterBounds.push_back(Primitive.mProxy.mBounds);
        }
    }
    TArray<Uint32> Order{};
    Order.resize(Scene.GetLights().size());
    std::iota(Order.begin(), Order.end(), 0u);
    std::stable_sort(Order.begin(), Order.end(), [&Scene](Uint32 First, Uint32 Second) {
        const FLightSceneProxy& A{Scene.GetLights()[First]};
        const FLightSceneProxy& B{Scene.GetLights()[Second]};
        return A.mShadowResolution * A.mShadowResolutionScale > B.mShadowResolution * B.mShadowResolutionScale;
    });
    bool HasDirectional{};
    for (Uint32 LightIndex : Order) {
        const FLightSceneProxy& Light{Scene.GetLights()[LightIndex]};
        if (!Light.mVisible || Light.mIntensity <= 0.0f || !Light.mCastShadows || Light.mType == ELightType::Ambient) {
            continue;
        }
        if (Light.mType == ELightType::Directional && HasDirectional) {
            ++Stats.mShadowDroppedLights;
            continue;
        }
        const Uint32 Count{Light.mType == ELightType::Point ? 6u : Light.mType == ELightType::Directional ? std::clamp(Light.mCascadeCount, 1u, 4u) : 1u};
        const Uint32 Requested{std::bit_ceil(static_cast<Uint32>(std::clamp(Light.mShadowResolution * Light.mShadowResolutionScale, 64.0f, 8192.0f)))};
        Uint32 Size{Requested};
        TArray<FShadowAtlasBlock> Blocks{};
        const FShadowAtlasAllocator Saved{mAllocator};
        while (Size >= 64) {
            mAllocator = Saved;
            Blocks.clear();
            for (Uint32 Face{}; Face < Count; ++Face) {
                const auto Block{mAllocator.Allocate(Size)};
                if (!Block.has_value()) {
                    break;
                }
                Blocks.push_back(*Block);
            }
            if (Blocks.size() == Count) {
                break;
            }
            Size /= 2;
        }
        if (Blocks.size() != Count) {
            mAllocator = Saved;
            ++Stats.mShadowDroppedLights;
            continue;
        }
        Stats.mShadowReducedLights += Size < Requested;
        HasDirectional = HasDirectional || Light.mType == ELightType::Directional;
        mLightRanges[LightIndex] = {static_cast<Uint32>(mMaps.size()) + 1, Count};
        const FSceneView& View{Family.mView};
        const float ProjectionDepth{View.mProjection(2, 2)};
        const float CameraNear{std::max(0.001f, -View.mProjection(3, 2) / ProjectionDepth)};
        const float CameraFar{View.mOrthographic ? (1.0f - View.mProjection(3, 2)) / ProjectionDepth : ProjectionDepth > 1.0f ? -View.mProjection(3, 2) / (ProjectionDepth - 1.0f) : Light.mShadowDistance};
        const float FarDistance{std::max(CameraNear + 0.01f, std::min(CameraFar, Light.mShadowDistance))};
        float PreviousSplit{CameraNear};
        float PreviousBlend{CameraNear};
        for (Uint32 Face{}; Face < Count; ++Face) {
            const FShadowAtlasBlock& Block{Blocks[Face]};
            const Uint32 Resolution{Block.mSize - Guard * 2};
            FGpuShadow Map{};
            Map.mAtlasRectangle = {static_cast<float>(Block.mX + Guard) / AtlasSize, static_cast<float>(Block.mY + Guard) / AtlasSize, static_cast<float>(Resolution) / AtlasSize, static_cast<float>(Resolution) / AtlasSize};
            Map.mParameters = {Light.mShadowBias, Light.mShadowSlopeBias, Light.mShadowSharpen, 1.0f / AtlasSize};
            if (Light.mType == ELightType::Directional) {
                const float Fraction{static_cast<float>(Face + 1) / Count};
                const float Linear{CameraNear + (FarDistance - CameraNear) * Fraction};
                const float Logarithmic{CameraNear * std::pow(FarDistance / CameraNear, Fraction)};
                const float Split{View.mOrthographic ? Linear : Linear * 0.35f + Logarithmic * 0.65f};
                const float BlendStart{Split - (Split - PreviousSplit) * std::clamp(Light.mCascadeTransitionFraction, 0.0f, 0.5f)};
                Map.mViewProjection = MakeDirectionalMatrix(View, Light.mDirection, PreviousBlend, Split, Resolution, CasterBounds);
                Map.mCascade = {PreviousSplit, Split, BlendStart, 0.0f};
                PreviousBlend = BlendStart;
                PreviousSplit = Split;
            } else {
                const std::array<FVector, 6> Directions{{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}}};
                const FVector Direction{Light.mType == ELightType::Point ? Directions[Face] : Light.mDirection};
                const float NearPlane{std::min(0.05f, Light.mAttenuationRadius * 0.01f)};
                const float FieldOfView{Light.mType == ELightType::Point ? 90.0f : std::clamp(Light.mOuterConeAngle * 2.0f, 1.0f, 179.0f)};
                Map.mViewProjection = FMatrix::LookTo(Light.mPosition, Direction, LightUp(Direction)) * FMatrix::Perspective(FieldOfView, 1.0f, NearPlane, Light.mAttenuationRadius);
            }
            mMaps.push_back(Map);
            mTiles.push_back({Light.mId, {static_cast<float>(Block.mX) / AtlasSize, static_cast<float>(Block.mY) / AtlasSize, static_cast<float>(Block.mSize) / AtlasSize, static_cast<float>(Block.mSize) / AtlasSize}, Face, Resolution});
            Stats.mShadowAllocatedBytes += static_cast<std::size_t>(Block.mSize) * Block.mSize * 4;
        }
    }
    Stats.mShadowMaps = static_cast<Uint32>(mMaps.size());
}

bool FShadowResources::Upload(ID3D11Device* Device, ID3D11DeviceContext* Context) {
    const Uint32 Count{static_cast<Uint32>(mMaps.size())};
    if (mCapacity < std::max(Count, 1u)) {
        mShadowView.Reset();
        mShadowBuffer.Reset();
        mCapacity = 0;
        D3D11_BUFFER_DESC Buffer{};
        Buffer.ByteWidth = std::max(Count, 1u) * sizeof(FGpuShadow);
        Buffer.Usage = D3D11_USAGE_DEFAULT;
        Buffer.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        Buffer.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        Buffer.StructureByteStride = sizeof(FGpuShadow);
        if (FAILED(Device->CreateBuffer(&Buffer, nullptr, mShadowBuffer.GetAddressOf())) || FAILED(Device->CreateShaderResourceView(mShadowBuffer.Get(), nullptr, mShadowView.GetAddressOf()))) {
            return false;
        }
        mCapacity = std::max(Count, 1u);
    }
    if (Count > 0) {
        const D3D11_BOX Box{0, 0, 0, Count * static_cast<Uint32>(sizeof(FGpuShadow)), 1, 1};
        Context->UpdateSubresource(mShadowBuffer.Get(), 0, &Box, mMaps.data(), 0, 0);
    }
    return true;
}

bool FShadowResources::Render(ID3D11Device* Device, ID3D11DeviceContext* Context, const FScene& Scene, const FSceneViewFamily& Family, const std::array<std::unique_ptr<UStaticMesh>, static_cast<std::size_t>(EPrimitiveType::Count)>& Meshes, FRenderStats& Stats) {
    Unbind(Context);
    if (mSampler == nullptr) {
        D3D11_SAMPLER_DESC Sampler{};
        Sampler.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        Sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        Sampler.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
        Sampler.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(Device->CreateSamplerState(&Sampler, mSampler.GetAddressOf()))) {
            return false;
        }
    }
    mEnabled = false;
    mMaps.clear();
    mTiles.clear();
    mLightRanges.assign(Scene.GetLights().size(), {});
    Stats.mDirectionalLights = 0;
    Stats.mPointLights = 0;
    Stats.mSpotLights = 0;
    Stats.mShadowMaps = 0;
    Stats.mShadowDrawCalls = 0;
    Stats.mShadowAllocatedBytes = 0;
    Stats.mShadowDroppedLights = 0;
    Stats.mShadowReducedLights = 0;
    for (const FLightSceneProxy& Light : Scene.GetLights()) {
        if (Light.mVisible && Light.mIntensity > 0.0f) {
            Stats.mDirectionalLights += Light.mType == ELightType::Directional;
            Stats.mPointLights += Light.mType == ELightType::Point;
            Stats.mSpotLights += Light.mType == ELightType::Spot;
        }
    }
    const bool Lit{Family.mViewMode == EViewMode::Lit_Gouraud || Family.mViewMode == EViewMode::Lit_Lambert || Family.mViewMode == EViewMode::Lit_Phong};
    if (Lit && Family.mShowFlags.mShadows && Family.mShowFlags.mPrimitives) {
        Build(Scene, Family, Stats);
    }
    if (mMaps.empty()) {
        Stats.mShadowAtlasBytes = mAtlas != nullptr ? mAtlas->GetResourceBytes() + mPreview->GetResourceBytes() : 0;
        return true;
    }
    if (!CreateResources(Device) || !Upload(Device, Context)) {
        mMaps.clear();
        mTiles.clear();
        mLightRanges.assign(Scene.GetLights().size(), {});
        return false;
    }
    Stats.mShadowAtlasBytes = mAtlas->GetResourceBytes() + mPreview->GetResourceBytes();
    Context->OMSetRenderTargets(0, nullptr, mAtlas->GetDepthStencilView());
    Context->ClearDepthStencilView(mAtlas->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    Context->OMSetDepthStencilState(mDepthState.Get(), 0);
    Context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
    Context->RSSetState(mRasterizer.Get());
    Context->VSSetShader(mDepthShader.Get(), nullptr, 0);
    Context->PSSetShader(nullptr, nullptr, 0);
    Context->GSSetShader(nullptr, nullptr, 0);
    Context->HSSetShader(nullptr, nullptr, 0);
    Context->DSSetShader(nullptr, nullptr, 0);
    Context->IASetInputLayout(mLayout.Get());
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11Buffer* MapBuffer{mMapBuffer.Get()};
    Context->VSSetConstantBuffers(1, 1, &MapBuffer);
    for (const FGpuShadow& Map : mMaps) {
        Context->UpdateSubresource(mMapBuffer.Get(), 0, nullptr, &Map.mViewProjection, 0, 0);
        const D3D11_VIEWPORT Viewport{Map.mAtlasRectangle.x * AtlasSize, Map.mAtlasRectangle.y * AtlasSize, Map.mAtlasRectangle.z * AtlasSize, Map.mAtlasRectangle.w * AtlasSize, 0.0f, 1.0f};
        const D3D11_RECT Scissor{static_cast<LONG>(Viewport.TopLeftX), static_cast<LONG>(Viewport.TopLeftY), static_cast<LONG>(Viewport.TopLeftX + Viewport.Width), static_cast<LONG>(Viewport.TopLeftY + Viewport.Height)};
        Context->RSSetViewports(1, &Viewport);
        Context->RSSetScissorRects(1, &Scissor);
        for (const FScenePrimitive& Primitive : Scene.GetPrimitives()) {
            if (!Primitive.mProxy.mCastShadow || Primitive.mProxy.mSprite.has_value() || Primitive.mProxy.mText.has_value() || Primitive.mObjectBuffer == nullptr) {
                continue;
            }
            UStaticMesh* Mesh{Primitive.mProxy.mMesh != nullptr ? Primitive.mProxy.mMesh : Meshes[static_cast<std::size_t>(Primitive.mProxy.mPrimitive)].get()};
            if (Mesh == nullptr || (!Mesh->HasResources() && !Mesh->CreateResources(Device))) {
                continue;
            }
            ID3D11Buffer* Position{Mesh->GetPositionBuffer()};
            ID3D11Buffer* ObjectBuffer{Primitive.mObjectBuffer.Get()};
            const UINT Stride{UStaticMesh::GetPositionStride()};
            const UINT Offset{};
            Context->IASetVertexBuffers(0, 1, &Position, &Stride, &Offset);
            Context->IASetIndexBuffer(Mesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
            Context->VSSetConstantBuffers(0, 1, &ObjectBuffer);
            for (const FSubMesh& SubMesh : Mesh->GetMeshData().GetSubMeshes()) {
                Context->DrawIndexed(SubMesh.GetIndexCount(), SubMesh.GetFirstIndex(), 0);
                ++Stats.mShadowDrawCalls;
            }
        }
    }
    Context->OMSetRenderTargets(0, nullptr, nullptr);
    const std::array<FVector4, 2> ViewData{{{Family.mView.mLocation, 0.0f}, {Family.mView.mForward, 0.0f}}};
    Context->UpdateSubresource(mViewBuffer.Get(), 0, nullptr, ViewData.data(), 0, 0);
    RenderPreview(Context);
    mEnabled = true;
    return true;
}

void FShadowResources::RenderPreview(ID3D11DeviceContext* Context) {
    ID3D11RenderTargetView* Target{mPreview->GetRenderTargetView()};
    Context->OMSetRenderTargets(1, &Target, nullptr);
    Context->OMSetDepthStencilState(mPreviewDepthState.Get(), 0);
    const D3D11_VIEWPORT Viewport{0.0f, 0.0f, 2048.0f, 2048.0f, 0.0f, 1.0f};
    const D3D11_RECT Scissor{0, 0, 2048, 2048};
    Context->RSSetViewports(1, &Viewport);
    Context->RSSetScissorRects(1, &Scissor);
    Context->IASetInputLayout(nullptr);
    Context->VSSetShader(mPreviewVertexShader.Get(), nullptr, 0);
    Context->PSSetShader(mPreviewPixelShader.Get(), nullptr, 0);
    ID3D11ShaderResourceView* Atlas{mAtlas->GetShaderResourceView()};
    Context->PSSetShaderResources(5, 1, &Atlas);
    Context->Draw(3, 0);
    ID3D11ShaderResourceView* Empty{};
    Context->PSSetShaderResources(5, 1, &Empty);
    Context->OMSetRenderTargets(0, nullptr, nullptr);
}

void FShadowResources::Bind(ID3D11DeviceContext* Context) const {
    const std::array<ID3D11ShaderResourceView*, 2> Views{mEnabled ? mAtlas->GetShaderResourceView() : nullptr, mEnabled ? mShadowView.Get() : nullptr};
    Context->PSSetShaderResources(5, static_cast<UINT>(Views.size()), Views.data());
    ID3D11SamplerState* Sampler{mSampler.Get()};
    ID3D11Buffer* ViewBuffer{mViewBuffer.Get()};
    Context->PSSetSamplers(1, 1, &Sampler);
    Context->PSSetConstantBuffers(4, 1, &ViewBuffer);
}

void FShadowResources::Unbind(ID3D11DeviceContext* Context) const {
    const std::array<ID3D11ShaderResourceView*, 2> Empty{};
    Context->PSSetShaderResources(5, static_cast<UINT>(Empty.size()), Empty.data());
}

const TArray<std::array<Uint32, 2>>& FShadowResources::GetLightRanges() const {
    return mLightRanges;
}

FShadowAtlasView FShadowResources::GetView() const {
    return {mEnabled && mPreview != nullptr ? mPreview->GetShaderResourceView() : nullptr, AtlasSize, mTiles, mEnabled ? mAtlas->GetShaderResourceView() : nullptr};
}
