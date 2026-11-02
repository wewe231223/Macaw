/**
 * @file UberLit.hlsli
 * @brief 타일 광원·노멀 맵·그림자를 조합하는 공통 메시 픽셀 셰이더입니다.
 */

#include "LightingCommon.hlsli"

#if defined(MACAW_RECEIVE_SHADOWS) && !defined(LIGHTING_MODEL_GOURAUD) && !defined(MACAW_UNLIT) && !defined(WORLD_NORMAL)
#include "ShadowSampling.hlsli"
#endif

/**
 * @brief 행 우선 타일별 광원 인덱스 시작 위치와 개수입니다.
 */
StructuredBuffer<uint2> TileRanges : register(t3);

/**
 * @brief 타일별로 연속 저장한 광원 배열의 인덱스입니다.
 */
StructuredBuffer<uint> TileLightIndices : register(t4);

/**
 * @brief 재질·노멀 맵과 선택한 조명 모드·타일 광원·그림자를 조합합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain(FPixelInput Input) : SV_TARGET {
    float4 Surface = {BaseColor * Input.mColor * BaseColorTexture.Sample(MaterialSampler, Input.mUV)};
    float3 Normal = {SafeNormal(Input.mNormal)};
#ifdef HAS_NORMAL_MAP
    Normal = MappedNormal(Normal, Input.mTangent, NormalMapTexture.Sample(MaterialSampler, Input.mUV).xyz);
#endif
#if defined(WORLD_NORMAL)
    return float4(Normal * 0.5f + 0.5f, 1.0f);
#elif defined(MACAW_UNLIT)
    return Surface;
#else
    FLighting Total = {(FLighting)0};
    float3 SpecularTint = {SpecularColor.rgb};
    float3 Emissive = {EmissiveColor.rgb};

    if (TextureFlags.x > 0.0f) {
        SpecularTint *= SpecularTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

    if (TextureFlags.y > 0.0f) {
        Emissive *= EmissiveTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

#if defined(LIGHTING_MODEL_GOURAUD)
    Total.mDiffuse = Input.mDiffuse;
    Total.mSpecular = Input.mSpecular;
    Total.mAmbient = Input.mAmbient;
#else
#if defined(MACAW_RECEIVE_SHADOWS)
    float3 PositionDx = {ddx(Input.mWorldPosition)};
    float3 PositionDy = {ddy(Input.mWorldPosition)};
#endif
    bool Tiled = {(TileDimensions.w & 0x80000000u) == 0};
    uint2 Range = {uint2(0, TileDimensions.w & 0x7fffffffu)};

    [branch]
    if (Tiled) {
        uint2 Tile = {min((uint2)Input.mPosition.xy / TileDimensions.z, TileDimensions.xy - 1)};
        Range = TileRanges[Tile.y * TileDimensions.x + Tile.x];
    }

    for (uint Offset = {0}; Offset < Range.y; ++Offset) {
        uint LightIndex = {Offset};

        [branch]
        if (Tiled) {
            LightIndex = TileLightIndices[Range.x + Offset];
        }

        FLight Light = {Lights[LightIndex]};
        FLighting Lighting = {EvaluateLight(Light, Input.mWorldPosition, Normal)};
#if defined(MACAW_RECEIVE_SHADOWS)
        float Visibility = {1.0f};

        [branch]
        if (Light.mShadowInfo.x >= 1.0f && (any(Lighting.mDiffuse != 0.0f) || any(Lighting.mSpecular != 0.0f))) {
            Visibility = ShadowVisibility(Light, Input.mWorldPosition, Normal, PositionDx, PositionDy);
        }
#else
        float Visibility = {1.0f};
#endif
        Total.mDiffuse += Lighting.mDiffuse * Visibility;
        Total.mSpecular += Lighting.mSpecular * Visibility;
        Total.mAmbient += Lighting.mAmbient;
    }
#endif

    float3 Reflection = {float3(0.0f, 0.0f, 0.0f)};

#if defined(LIGHTING_MODEL_PHONG) || defined(LIGHTING_MODEL_GOURAUD)
    float3 Environment = {Total.mAmbient};

    if (TextureFlags.z > 0.0f) {
        float3 V = {SafeNormal(CameraAndLightCount.xyz - Input.mWorldPosition)};
        float3 R = {reflect(-V, Normal)};
        float2 ReflectionUV = {float2(atan2(R.y, R.x) * 0.159154943f + 0.5f, acos(clamp(R.z, -1.0f, 1.0f)) * 0.318309886f)};
        uint2 ReflectionSize = {uint2(0, 0)};
        ReflectionTexture.GetDimensions(ReflectionSize.x, ReflectionSize.y);

        float PoleMargin = {0.5f / max((float)ReflectionSize.y, 1.0f)};
        ReflectionUV.y = clamp(ReflectionUV.y, PoleMargin, 1.0f - PoleMargin);
        Environment = ReflectionTexture.SampleLevel(MaterialSampler, ReflectionUV, 0).rgb;
    }

    Reflection = Environment * ReflectionColor.rgb * SurfaceParameters.w;
#endif

    return float4(Surface.rgb * Total.mDiffuse + Total.mSpecular * SpecularTint + Emissive + Reflection, Surface.a);
#endif
}
