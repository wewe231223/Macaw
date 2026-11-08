/**
 * @file UberLit.hlsli
 * @brief 재질을 평가해 G-buffer 또는 단일 컬러 출력으로 기록합니다.
 */

#include "LightingCommon.hlsli"

#if !defined(MACAW_GBUFFER)
#include "PixelLighting.hlsli"
#endif

struct FSurfaceMaterial {
    float4 mColor;
    float3 mNormal;
    float3 mSpecularTint;
    float3 mEmissive;
    float3 mReflectionTint;
};

FSurfaceMaterial ReadSurfaceMaterial(FPixelInput Input) {
    FSurfaceMaterial Surface = {(FSurfaceMaterial)0};
    Surface.mColor = BaseColor * Input.mColor * BaseColorTexture.Sample(MaterialSampler, Input.mUV);
    Surface.mNormal = SafeNormal(Input.mNormal);

#if defined(HAS_NORMAL_MAP)
    Surface.mNormal = MappedNormal(Surface.mNormal, Input.mTangent, NormalMapTexture.Sample(MaterialSampler, Input.mUV).xyz);
#endif

#if !defined(MACAW_UNLIT) && !defined(WORLD_NORMAL)
    Surface.mSpecularTint = SpecularColor.rgb;
    Surface.mEmissive = EmissiveColor.rgb;

    if (TextureFlags.x > 0.0f) {
        Surface.mSpecularTint *= SpecularTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

    if (TextureFlags.y > 0.0f) {
        Surface.mEmissive *= EmissiveTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

#if defined(LIGHTING_MODEL_PHONG) || defined(LIGHTING_MODEL_GOURAUD)
    Surface.mReflectionTint = ReflectionColor.rgb * SurfaceParameters.w;

    if (TextureFlags.z > 0.0f) {
        const float3 ViewDirection = {SafeNormal(CameraAndLightCount.xyz - Input.mWorldPosition)};
        const float3 Reflected = {reflect(-ViewDirection, Surface.mNormal)};
        float2 ReflectionUV = {float2(atan2(Reflected.y, Reflected.x) * 0.159154943f + 0.5f, acos(clamp(Reflected.z, -1.0f, 1.0f)) * 0.318309886f)};
        uint2 ReflectionSize = {uint2(0, 0)};
        ReflectionTexture.GetDimensions(ReflectionSize.x, ReflectionSize.y);

        const float PoleMargin = {0.5f / max((float)ReflectionSize.y, 1.0f)};
        ReflectionUV.y = clamp(ReflectionUV.y, PoleMargin, 1.0f - PoleMargin);
        Surface.mEmissive += ReflectionTexture.SampleLevel(MaterialSampler, ReflectionUV, 0).rgb * Surface.mReflectionTint;
        Surface.mReflectionTint = 0.0f;
    }
#endif
#endif

    return Surface;
}

#if defined(MACAW_GBUFFER)
struct FGBufferOutput {
    float4 mColor : SV_TARGET0;
    float4 mNormal : SV_TARGET1;
    float4 mSpecular : SV_TARGET2;
    float4 mEmission : SV_TARGET3;
    float4 mAmbient : SV_TARGET4;
    float4 mReflection : SV_TARGET5;
};

FGBufferOutput PixelMain(FPixelInput Input) {
    const FSurfaceMaterial Surface = {ReadSurfaceMaterial(Input)};
    FGBufferOutput Output = {(FGBufferOutput)0};
    Output.mColor = Surface.mColor;
    Output.mNormal = float4(Surface.mNormal, SurfaceParameters.y);

#if defined(MACAW_UNLIT)
    Output.mNormal.w = -1.0f;
#elif defined(LIGHTING_MODEL_GOURAUD)
    Output.mAmbient.rgb = Input.mDiffuse;
    Output.mEmission.rgb = Surface.mEmissive + Input.mSpecular * Surface.mSpecularTint + Input.mAmbient * Surface.mReflectionTint;
#elif !defined(WORLD_NORMAL)
    Output.mSpecular.rgb = Surface.mSpecularTint * SurfaceParameters.x;
    Output.mEmission.rgb = Surface.mEmissive;
    Output.mAmbient.rgb = AmbientColor.rgb;
    Output.mReflection.rgb = Surface.mReflectionTint;

#if defined(MACAW_RECEIVE_SHADOWS)
    const float3 PlaneNormal = {normalize(cross(ddx(Input.mWorldPosition), ddy(Input.mWorldPosition)))};
    Output.mSpecular.w = 1.0f;
    Output.mEmission.w = PlaneNormal.x;
    Output.mAmbient.w = PlaneNormal.y;
    Output.mReflection.w = PlaneNormal.z;
#endif
#endif

    return Output;
}
#else
float4 PixelMain(FPixelInput Input) : SV_TARGET {
    const FSurfaceMaterial Surface = {ReadSurfaceMaterial(Input)};

#if defined(WORLD_NORMAL)
    return float4(Surface.mNormal * 0.5f + 0.5f, 1.0f);
#elif defined(MACAW_UNLIT)
    return Surface.mColor;
#else
#if defined(LIGHTING_MODEL_GOURAUD)
    const FLighting Total = {Input.mDiffuse, Input.mSpecular, Input.mAmbient};
#else
    const FLighting Total = {AccumulatePixelLighting(Input.mPosition.xy, Input.mWorldPosition, Surface.mNormal, AmbientColor.rgb, SurfaceParameters.xy, true, ddx(Input.mWorldPosition), ddy(Input.mWorldPosition))};
#endif

    const float3 Color = {Surface.mColor.rgb * Total.mDiffuse + Total.mSpecular * Surface.mSpecularTint + Surface.mEmissive + Total.mAmbient * Surface.mReflectionTint};

    return float4(Color, Surface.mColor.a);
#endif
}
#endif
