/**
 * @file UberLit.hlsli
 * @brief 타일 광원·노멀 맵·그림자를 조합하는 공통 메시 정점·픽셀 셰이더입니다.
 */

#include "MaterialCommon.hlsli"

/**
 * @brief GPU 타일 조명 계산에 사용하는 광원 정보입니다.
 */
struct FLight {
    /**
     * @brief 광원의 월드 위치 XYZ와 영향 반경 W입니다.
     */
    float4 mPositionAndRadius;

    /**
     * @brief 광원 방향 XYZ와 종류 번호 W입니다.
     */
    float4 mDirectionAndType;

    /**
     * @brief 광원의 RGB 색상과 밝기 배율 W입니다.
     */
    float4 mColorAndIntensity;

    /**
     * @brief 감쇠 지수와 내부·외부 반각의 코사인, 예약값입니다.
     */
    float4 mFalloffAndCones;

    /**
     * @brief 그림자 맵의 1 기반 시작 인덱스·개수와 예약값이며 시작값 0은 그림자 없음입니다.
     */
    float4 mShadowInfo;
};

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
     * @brief 타일 열·행 개수, 한 변의 픽셀 수와 예약값입니다.
     */
    uint4 TileDimensions;
};

/**
 * @brief 가시 광원을 GPU 형식으로 저장한 구조화 버퍼입니다.
 */
StructuredBuffer<FLight> Lights : register(t2);

/**
 * @brief 행 우선 타일별 광원 인덱스 시작 위치와 개수입니다.
 */
StructuredBuffer<uint2> TileRanges : register(t3);

/**
 * @brief 타일별로 연속 저장한 광원 배열의 인덱스입니다.
 */
StructuredBuffer<uint> TileLightIndices : register(t4);

/**
 * @brief 길이가 0에 가까운 벡터도 유한한 값으로 정규화합니다.
 * @param[in] Value 정규화할 벡터입니다.
 * @return 안전한 역길이를 곱한 방향 벡터이며 영벡터 입력은 영벡터입니다.
 */
float3 SafeNormal(float3 Value) {
    return Value * rsqrt(max(dot(Value, Value), 0.00000001));
}

#include "ShadowSampling.hlsli"

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
FLighting EvaluateLight(FLight Light, float3 Position, float3 Normal) {
    FLighting Result = {(FLighting)0};
    float3 Radiance = {Light.mColorAndIntensity.rgb * Light.mColorAndIntensity.w};
    uint Type = {(uint)Light.mDirectionAndType.w};

    if (Type == 0) {
        Result.mDiffuse = Radiance * AmbientColor.rgb;
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
    float Specular = {Diffuse > 0.0f ? pow(saturate(dot(Normal, H)), SurfaceParameters.y) * SurfaceParameters.x : 0.0f};
    Result.mSpecular = Radiance * (Attenuation * Specular);
#endif

    return Result;
}

/**
 * @brief 입력 정점의 클립 위치와 다음 단계의 보간 데이터를 계산합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FOutput VertexMain(FInput Input) {
    FOutput Output = {(FOutput)0};

#ifdef MacawInstanced
    row_major float4x4 ObjectWorld = {float4x4(Input.mWorld0, Input.mWorld1, Input.mWorld2, Input.mWorld3)};
    row_major float4x4 ObjectInverse = {float4x4(Input.mInverse0, Input.mInverse1, Input.mInverse2, Input.mInverse3)};
    float4 ObjectUV = {Input.mUVTransform};

    Output.mReceiveShadow = Input.mShadowFlags.x;
#else
    row_major float4x4 ObjectWorld = {World};
    row_major float4x4 ObjectInverse = {WorldInverse};
    float4 ObjectUV = {UVTransform};

    Output.mReceiveShadow = ShadowFlags.x;
#endif

    Output.mWorldPosition = mul(float4(Input.mPosition, 1.0f), ObjectWorld).xyz;
    Output.mPosition = mul(float4(Output.mWorldPosition, 1.0f), ViewProjection);
    Output.mNormal = SafeNormal(mul(Input.mNormal, transpose((float3x3)ObjectInverse)));
    Output.mTangent = float4(SafeNormal(mul(Input.mTangent.xyz, (float3x3)ObjectWorld)), Input.mTangent.w * (determinant((float3x3)ObjectWorld) < 0.0f ? -1.0f : 1.0f));
    Output.mUV = Input.mUV * ObjectUV.xy + ObjectUV.zw;
    Output.mColor = Input.mColor;

#if defined(LIGHTING_MODEL_GOURAUD) && !defined(MACAW_UNLIT) && !defined(WORLD_NORMAL)
    float3 Normal = {Output.mNormal};

#ifdef HAS_NORMAL_MAP
    Normal = MappedNormal(Normal, Output.mTangent, NormalMapTexture.SampleLevel(MaterialSampler, Output.mUV, 0).xyz);
#endif

    for (uint Index = {0}; Index < (uint)CameraAndLightCount.w; ++Index) {
        FLighting Lighting = {EvaluateLight(Lights[Index], Output.mWorldPosition, Normal)};
        Output.mDiffuse += Lighting.mDiffuse;
        Output.mSpecular += Lighting.mSpecular;
    }
#endif

    return Output;
}

/**
 * @brief 재질·노멀 맵과 선택한 조명 모드·타일 광원·그림자를 조합합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain(FOutput Input) : SV_TARGET {
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
    FLighting Unshadowed = {(FLighting)0};
    float3 SpecularTint = {SpecularColor.rgb};
    float3 Emissive = {EmissiveColor.rgb};

    if (TextureFlags.x > 0.0f) {
        SpecularTint *= SpecularTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

    if (TextureFlags.y > 0.0f) {
        Emissive *= EmissiveTexture.Sample(MaterialSampler, Input.mUV).rgb;
    }

    float3 PositionDx = {ddx(Input.mWorldPosition)};
    float3 PositionDy = {ddy(Input.mWorldPosition)};
    uint2 Tile = {min((uint2)Input.mPosition.xy / TileDimensions.z, TileDimensions.xy - 1)};
    uint2 Range = {TileRanges[Tile.y * TileDimensions.x + Tile.x]};
    for (uint Offset = {0}; Offset < Range.y; ++Offset) {
        FLight Light = {Lights[TileLightIndices[Range.x + Offset]]};
        FLighting Lighting = {EvaluateLight(Light, Input.mWorldPosition, Normal)};
        float Visibility = {ShadowVisibility(Light, Input.mWorldPosition, Normal, PositionDx, PositionDy, Input.mReceiveShadow)};
        Unshadowed.mDiffuse += Lighting.mDiffuse;
        Unshadowed.mSpecular += Lighting.mSpecular;
        Total.mDiffuse += Lighting.mDiffuse * Visibility;
        Total.mSpecular += Lighting.mSpecular * Visibility;
        Total.mAmbient += Lighting.mAmbient;
    }

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

#if defined(LIGHTING_MODEL_GOURAUD)
    float3 DiffuseRatio = {saturate((Total.mDiffuse + 0.000001f) / (Unshadowed.mDiffuse + 0.000001f))};
    float3 SpecularRatio = {saturate((Total.mSpecular + 0.000001f) / (Unshadowed.mSpecular + 0.000001f))};
    return float4(Surface.rgb * Input.mDiffuse * DiffuseRatio + Input.mSpecular * SpecularRatio * SpecularTint + Emissive + Reflection, Surface.a);
#else
    return float4(Surface.rgb * Total.mDiffuse + Total.mSpecular * SpecularTint + Emissive + Reflection, Surface.a);
#endif
#endif
}
