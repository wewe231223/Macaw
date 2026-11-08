#ifndef MacawLightingCommonIncluded
#define MacawLightingCommonIncluded

#include "MaterialCommon.hlsli"
#include "LightData.hlsli"

/**
 * @brief 분리하여 누적하는 RGB 확산·정반사 조명 결과입니다.
 */
struct FLighting {
    /**
     * @brief 광원 계산으로 누적한 RGB 확산 조명입니다.
     */
    float3 mDiffuse;

    /**
     * @brief 광원 계산으로 누적한 RGB 정반사 조명입니다.
     */
    float3 mSpecular;
    float3 mAmbient;
};

/**
 * @brief 타일 조명의 카메라·광원 개수·격자 크기 상수입니다.
 */
cbuffer LightingConstants : register(b3) {
    /**
     * @brief 카메라 월드 위치 XYZ와 GPU 광원 개수 W입니다.
     */
    float4 CameraAndLightCount;

    /**
     * @brief 타일 열·행 개수, 한 변의 픽셀 수와 광원 개수이며 마지막 값의 최상위 비트는 전체 광원 순회 여부입니다.
     */
    uint4 TileDimensions;
};

/**
 * @brief 가시 광원을 GPU 형식으로 저장한 구조화 버퍼입니다.
 */
StructuredBuffer<FLight> Lights : register(t2);

/**
 * @brief 길이가 0에 가까운 벡터도 유한한 값으로 정규화합니다.
 * @param[in] Value 정규화할 벡터입니다.
 * @return 안전한 역길이를 곱한 방향 벡터이며 영벡터 입력은 영벡터입니다.
 */
float3 SafeNormal(float3 Value) {
    return Value * rsqrt(max(dot(Value, Value), 0.00000001));
}

/**
 * @brief 접선·종법선 기저와 재질 강도로 노멀 맵을 월드 법선으로 변환합니다.
 * @param[in] Normal 월드 공간 표면 법선입니다.
 * @param[in] Tangent 월드 접선 XYZ와 종법선 부호 W입니다.
 * @param[in] Sampled 노멀 맵에서 읽은 0~1 범위의 RGB 값입니다.
 * @return 정규화된 월드 공간 법선입니다.
 */
float3 MappedNormal(float3 Normal, float4 Tangent, float3 Sampled) {
    float3 N = {SafeNormal(Normal)};
    float3 T = {SafeNormal(Tangent.xyz - N * dot(N, Tangent.xyz))};
    float3 B = {cross(N, T) * Tangent.w};
    float3 Map = {Sampled * 2.0f - 1.0f};
    Map.xy *= SurfaceParameters.z;

    return SafeNormal(T * Map.x + B * Map.y + N * Map.z);
}

/**
 * @brief 단일 광원의 감쇠·원뿔·조명 모델에 따른 확산·정반사 성분을 계산합니다.
 * @param[in] Light 평가할 광원의 GPU 정보입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] Normal 월드 공간 표면 법선입니다.
 * @return 그림자 가시성을 곱하기 전의 RGB 조명 성분입니다.
 */
FLighting EvaluateLight(FLight Light, float3 Position, float3 Normal, float3 AmbientTint, float2 SpecularParameters) {
    FLighting Result = {(FLighting)0};
    float3 Radiance = {Light.mColorAndIntensity.rgb * Light.mColorAndIntensity.w};
    uint Type = {(uint)Light.mDirectionAndType.w};

    if (Type == 0) {
        Result.mDiffuse = Radiance * AmbientTint;
        Result.mAmbient = Radiance;
        return Result;
    }

    float3 L = {-Light.mDirectionAndType.xyz};
    float Attenuation = {1.0f};

    if (Type >= 2) {
        float3 Offset = {Light.mPositionAndRadius.xyz - Position};
        float Distance = {length(Offset)};

        if (Distance >= Light.mPositionAndRadius.w) {
            return Result;
        }

        L = SafeNormal(Offset);

        float Radial = {saturate(1.0f - Distance / max(Light.mPositionAndRadius.w, 0.0001f))};
        Attenuation = Light.mFalloffAndCones.x > 0.0f ? pow(Radial, Light.mFalloffAndCones.x) : 1.0f;

        if (Type == 3) {
            float Cosine = {dot(-L, Light.mDirectionAndType.xyz)};
            float Inner = {Light.mFalloffAndCones.y};
            float Outer = {Light.mFalloffAndCones.z};
            float Cone = {Inner - Outer > 0.00001f ? saturate((Cosine - Outer) / (Inner - Outer)) : step(Outer, Cosine)};
            Attenuation *= Cone * Cone * (3.0f - 2.0f * Cone);
        }
    }

    float Diffuse = {saturate(dot(Normal, L))};
    Result.mDiffuse = Radiance * (Attenuation * Diffuse);

#if defined(LIGHTING_MODEL_PHONG) || defined(LIGHTING_MODEL_GOURAUD)
    float3 V = {SafeNormal(CameraAndLightCount.xyz - Position)};
    float3 H = {SafeNormal(L + V)};
    float Specular = {Diffuse > 0.0f && SpecularParameters.x > 0.0f ? pow(saturate(dot(Normal, H)), SpecularParameters.y) * SpecularParameters.x : 0.0f};
    Result.mSpecular = Radiance * (Attenuation * Specular);
#endif

    return Result;
}

FLighting EvaluateLight(FLight Light, float3 Position, float3 Normal) {
    return EvaluateLight(Light, Position, Normal, AmbientColor.rgb, SurfaceParameters.xy);
}

#endif
