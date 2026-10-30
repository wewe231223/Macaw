/**
 * @file ShadowSampling.hlsli
 * @brief 아틀라스 PCF 비교와 캐스케이드·큐브 면 선택으로 그림자 가시성을 계산합니다.
 */

/**
 * @brief 그림자 면·캐스케이드의 변환과 아틀라스 샘플링 정보입니다.
 */
struct FShadowMap {
    /**
     * @brief 월드 위치를 그림자 클립 공간으로 변환하는 행 우선 행렬입니다.
     */
    row_major float4x4 mViewProjection;

    /**
     * @brief 보호 여백을 제외한 아틀라스 UV 시작점 XY와 크기 ZW입니다.
     */
    float4 mAtlasRectangle;

    /**
     * @brief 깊이 바이어스·기울기 바이어스·선명도·아틀라스 역크기입니다.
     */
    float4 mParameters;

    /**
     * @brief 캐스케이드 시작·끝 거리와 혼합 시작 거리, 예약값입니다.
     */
    float4 mCascade;
};

/**
 * @brief 광원별 깊이 맵을 저장하는 그림자 아틀라스입니다.
 */
Texture2D<float> ShadowAtlas : register(t5);

/**
 * @brief 그림자 면·캐스케이드의 변환과 샘플링 정보입니다.
 */
StructuredBuffer<FShadowMap> ShadowMaps : register(t6);

/**
 * @brief 그림자 깊이를 비교하는 필터 샘플러입니다.
 */
SamplerComparisonState ShadowSampler : register(s1);

/**
 * @brief 그림자 캐스케이드 선택에 사용하는 카메라 상수입니다.
 */
cbuffer ShadowViewConstants : register(b4) {
    /**
     * @brief 캐스케이드 깊이 판정의 기준 카메라 위치와 예약값입니다.
     */
    float4 ShadowCamera;

    /**
     * @brief 캐스케이드 깊이를 투영할 카메라 전방 방향과 예약값입니다.
     */
    float4 ShadowForward;
};

/**
 * @brief 수신 표면의 기울기 바이어스와 3×3 PCF로 그림자 맵 하나를 샘플링합니다.
 * @param[in] Index 그림자 맵 구조화 버퍼의 0 기반 인덱스입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] Normal 월드 공간 표면 법선입니다.
 * @param[in] LightDirection 표면에서 광원을 향하는 단위 방향입니다.
 * @param[in] PositionDx 화면 X축에 대한 월드 위치 미분입니다.
 * @param[in] PositionDy 화면 Y축에 대한 월드 위치 미분입니다.
 * @return 0은 완전한 그림자, 1은 완전한 가시 상태인 계수이며 맵 밖은 1입니다.
 */
float SampleShadowMap(uint Index, float3 Position, float3 Normal, float3 LightDirection, float3 PositionDx, float3 PositionDy) {
    FShadowMap Map = {ShadowMaps[Index]};
    float4 Clip = {mul(float4(Position, 1.0f), Map.mViewProjection)};

    if (Clip.w <= 0.0f) {
        return 1.0f;
    }

    float3 Projected = {Clip.xyz / Clip.w};
    float2 UV = {Projected.xy * float2(0.5f, -0.5f) + 0.5f};

    if (any(UV < 0.0f) || any(UV > 1.0f) || Projected.z < 0.0f || Projected.z > 1.0f) {
        return 1.0f;
    }

    float Cosine = {saturate(dot(Normal, LightDirection))};
    float Slope = {min(sqrt(saturate(1.0f - Cosine * Cosine)) / max(Cosine, 0.05f), 16.0f)};
    float4 ClipDx = {mul(float4(Position + PositionDx, 1.0f), Map.mViewProjection)};
    float4 ClipDy = {mul(float4(Position + PositionDy, 1.0f), Map.mViewProjection)};
    float3 ProjectedDx = {ClipDx.xyz / ClipDx.w - Projected};
    float3 ProjectedDy = {ClipDy.xyz / ClipDy.w - Projected};
    float2 UVdx = {ProjectedDx.xy * float2(0.5f, -0.5f)};
    float2 UVdy = {ProjectedDy.xy * float2(0.5f, -0.5f)};
    float Depthdx = {ProjectedDx.z};
    float Depthdy = {ProjectedDy.z};
    float Determinant = {UVdx.x * UVdy.y - UVdx.y * UVdy.x};
    float2 Gradient = {abs(Determinant) > 0.00000001f ? float2(Depthdx * UVdy.y - Depthdy * UVdx.y, Depthdy * UVdx.x - Depthdx * UVdy.x) / Determinant : 0.0f};
    float ReceiverSlope = {(abs(Gradient.x) + abs(Gradient.y)) * Map.mParameters.w / Map.mAtlasRectangle.z};
    float Bias = {Map.mParameters.x + Map.mParameters.y * max(ReceiverSlope * 1.5f, Slope * 0.0001f)};
    float2 AtlasUV = {Map.mAtlasRectangle.xy + UV * Map.mAtlasRectangle.zw};
    float Texel = {Map.mParameters.w};
    float2 MinimumUV = {Map.mAtlasRectangle.xy + Texel * 0.5f};
    float2 MaximumUV = {Map.mAtlasRectangle.xy + Map.mAtlasRectangle.zw - Texel * 0.5f};
    float Visibility = {0.0f};

    [unroll]
    for (int Y = {-1}; Y <= 1; ++Y) {
        [unroll]
        for (int X = {-1}; X <= 1; ++X) {
            Visibility += ShadowAtlas.SampleCmpLevelZero(ShadowSampler, clamp(AtlasUV + float2(X, Y) * Texel, MinimumUV, MaximumUV), Projected.z - Bias);
        }
    }

    Visibility /= 9.0f;

    return saturate((Visibility - 0.5f) * (1.0f + Map.mParameters.z) + 0.5f);
}

/**
 * @brief 광원 종류에 맞는 그림자 면·캐스케이드를 선택하고 전환 구간을 혼합합니다.
 * @param[in] Light 평가할 광원의 GPU 정보입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] Normal 월드 공간 표면 법선입니다.
 * @param[in] PositionDx 화면 X축에 대한 월드 위치 미분입니다.
 * @param[in] PositionDy 화면 Y축에 대한 월드 위치 미분입니다.
 * @param[in] ReceiveShadow 0.5 이상이면 그림자를 수신합니다.
 * @return 광원의 그림자 가시성 계수이며 그림자가 없거나 수신을 끄면 1입니다.
 */
float ShadowVisibility(FLight Light, float3 Position, float3 Normal, float3 PositionDx, float3 PositionDy, float ReceiveShadow) {
    if (Light.mShadowInfo.x < 1.0f || ReceiveShadow < 0.5f) {
        return 1.0f;
    }

    uint Start = {(uint)Light.mShadowInfo.x - 1};
    uint Count = {(uint)Light.mShadowInfo.y};
    uint Type = {(uint)Light.mDirectionAndType.w};
    float3 L = {Type >= 2 ? SafeNormal(Light.mPositionAndRadius.xyz - Position) : -Light.mDirectionAndType.xyz};

    if (Type == 1) {
        float Depth = {dot(Position - ShadowCamera.xyz, ShadowForward.xyz)};

        for (uint Cascade = {0}; Cascade < Count; ++Cascade) {
            FShadowMap Map = {ShadowMaps[Start + Cascade]};

            if (Depth <= Map.mCascade.y) {
                float Current = {SampleShadowMap(Start + Cascade, Position, Normal, L, PositionDx, PositionDy)};

                if (Depth > Map.mCascade.z && Map.mCascade.y > Map.mCascade.z) {
                    float Next = {Cascade + 1 < Count ? SampleShadowMap(Start + Cascade + 1, Position, Normal, L, PositionDx, PositionDy) : 1.0f};
                    float Blend = {smoothstep(Map.mCascade.z, Map.mCascade.y, Depth)};
                    return lerp(Current, Next, Blend);
                }

                return Current;
            }
        }

        return 1.0f;
    }

    uint Face = {0};

    if (Type == 2) {
        float3 Offset = {Position - Light.mPositionAndRadius.xyz};
        float3 Magnitude = {abs(Offset)};

        if (Magnitude.x >= Magnitude.y && Magnitude.x >= Magnitude.z) {
            Face = Offset.x >= 0.0f ? 0 : 1;
        } else if (Magnitude.y >= Magnitude.z) {
            Face = Offset.y >= 0.0f ? 2 : 3;
        } else {
            Face = Offset.z >= 0.0f ? 4 : 5;
        }
    }

    return SampleShadowMap(Start + Face, Position, Normal, L, PositionDx, PositionDy);
}

/**
 * @brief 광원 종류에 맞는 그림자 면·캐스케이드를 선택하고 전환 구간을 혼합합니다.
 * @param[in] Light 평가할 광원의 GPU 정보입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] Normal 월드 공간 표면 법선입니다.
 * @param[in] PositionDx 화면 X축에 대한 월드 위치 미분입니다.
 * @param[in] PositionDy 화면 Y축에 대한 월드 위치 미분입니다.
 * @return 광원의 그림자 가시성 계수이며 그림자가 없거나 수신을 끄면 1입니다.
 * @note 그림자 수신 여부는 현재 객체 상수 ShadowFlags.x를 사용합니다.
 */
float ShadowVisibility(FLight Light, float3 Position, float3 Normal, float3 PositionDx, float3 PositionDy) {
    return ShadowVisibility(Light, Position, Normal, PositionDx, PositionDy, ShadowFlags.x);
}
