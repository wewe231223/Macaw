/**
 * @file ShadowSampling.hlsli
 * @brief 하드·PCF·PCSS 필터와 캐스케이드·큐브 면 선택으로 그림자 가시성을 계산합니다.
 */

// RenderCore::EShadowFilter와 순서가 같아야 합니다.
#define SHADOW_FILTER_HARD 0
#define SHADOW_FILTER_PCF 1
#define SHADOW_FILTER_PCSS 2

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

    float mSharpen;
    float mInverseAtlasSize;
    float2 mPadding;

    /**
     * @brief 캐스케이드 시작·끝 거리와 혼합 시작 거리, 예약값입니다.
     */
    float4 mCascade;

    /**
     * @brief PCSS의 광원 크기·타일 UV 배율·근평면·원평면 또는 깊이 범위이며 근평면이 0이면 방향광입니다.
     */
    float4 mSoftness;
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

    /**
     * @brief 필터 방식·PCF 격자 반경(탭)·PCSS 필터 반경 상한(텍셀)과 예약값입니다.
     */
    float4 ShadowFilter;
};

/**
 * @brief PCSS의 가림 물체 탐색과 필터에 쓰는 반지름 1 이내의 Poisson 원반 탭입니다.
 */
static const float2 ShadowPoisson[16] = {
    float2(-0.7634f, -0.3234f), float2(0.7663f, -0.6231f), float2(-0.0763f, -0.7531f), float2(0.2795f, 0.2381f),
    float2(-0.7422f, 0.3709f), float2(-0.6608f, -0.7124f), float2(-0.3102f, 0.2243f), float2(0.7900f, 0.6130f),
    float2(0.3592f, -0.7902f), float2(0.4355f, -0.3839f), float2(-0.2147f, -0.3395f), float2(0.6418f, 0.1547f),
    float2(-0.1960f, 0.8080f), float2(-0.6597f, 0.7410f), float2(0.1619f, 0.6373f), float2(0.1166f, -0.1143f)
};

/**
 * @brief 비교 샘플러로 지정한 반경의 Poisson 원반을 평균합니다.
 * @param[in] AtlasUV 필터 중심의 아틀라스 UV입니다.
 * @param[in] Reference 필터 중심의 비교 깊이입니다.
 * @param[in] Radius 아틀라스 UV 단위의 필터 반경입니다.
 * @param[in] Rotation 탭 패턴의 픽셀별 회전 행렬입니다.
 * @param[in] MinimumUV 타일 안쪽의 최소 아틀라스 UV입니다.
 * @param[in] MaximumUV 타일 안쪽의 최대 아틀라스 UV입니다.
 * @return 평균 가시성입니다.
 */
float FilterPoissonShadow(float2 AtlasUV, float Reference, float Radius, float2x2 Rotation, float2 MinimumUV, float2 MaximumUV) {
    float Visibility = {0.0f};

    [loop]
    for (uint Tap = {0}; Tap < 16; ++Tap) {
        float2 Offset = {mul(Rotation, ShadowPoisson[Tap]) * Radius};
        Visibility += ShadowAtlas.SampleCmpLevelZero(ShadowSampler, clamp(AtlasUV + Offset, MinimumUV, MaximumUV), Reference);
    }

    return Visibility / 16.0f;
}

/**
 * @brief 가림 물체를 탐색해 광원 크기와 거리 비로 반그림자 폭을 정하고 가변 반경으로 필터링합니다.
 * @param[in] AtlasUV 필터 중심의 아틀라스 UV입니다.
 * @param[in] ReceiverDepth 수신면의 투영 깊이입니다.
 * @param[in] PixelPosition 탭 회전에 쓰는 화면 픽셀 위치입니다.
 * @param[in] Map 광원 크기와 타일 영역을 가진 그림자 맵 정보입니다.
 * @param[in] MinimumUV 타일 안쪽의 최소 아틀라스 UV입니다.
 * @param[in] MaximumUV 타일 안쪽의 최대 아틀라스 UV입니다.
 * @return 평균 가시성이며 주변에 가림 물체가 없으면 1입니다.
 */
float FilterPCSS(float2 AtlasUV, float ReceiverDepth, float2 PixelPosition, FShadowMap Map, float2 MinimumUV, float2 MaximumUV) {
    float Texel = {Map.mInverseAtlasSize};
    float MaximumRadius = {Texel * ShadowFilter.z};
    float NearPlane = {Map.mSoftness.z};
    float FarPlane = {Map.mSoftness.w};
    bool Perspective = {NearPlane > 0.0f};
    float ReceiverDistance = {Perspective ? NearPlane * FarPlane / (FarPlane - ReceiverDepth * (FarPlane - NearPlane)) : ReceiverDepth * FarPlane};
    float SearchWorld = {Perspective ? Map.mSoftness.x * (ReceiverDistance - NearPlane) / (ReceiverDistance * NearPlane) : Map.mSoftness.x * ReceiverDistance};
    // 탐색 반경이 필터 상한보다 넓어야 반그림자 바깥 끝에서 가림 물체를 놓치지 않습니다.
    float SearchRadius = {clamp(SearchWorld * Map.mSoftness.y * Map.mAtlasRectangle.z, Texel, MaximumRadius * 1.5f)};
    float Noise = {frac(52.9829189f * frac(dot(PixelPosition, float2(0.06711056f, 0.00583715f))))};
    float Sine = {0.0f};
    float Cosine = {0.0f};
    sincos(Noise * 6.2831853f, Sine, Cosine);
    float2x2 Rotation = {Cosine, -Sine, Sine, Cosine};
    float AtlasSize = {1.0f / Texel};
    float BlockerSum = {0.0f};
    float BlockerCount = {0.0f};

    [loop]
    for (uint Tap = {0}; Tap < 16; ++Tap) {
        float2 Offset = {mul(Rotation, ShadowPoisson[Tap]) * SearchRadius};
        float2 TapUV = {clamp(AtlasUV + Offset, MinimumUV, MaximumUV)};
        float Depth = {ShadowAtlas.Load(int3(TapUV * AtlasSize, 0))};

        if (Depth < ReceiverDepth) {
            BlockerSum += Depth;
            BlockerCount += 1.0f;
        }
    }

    if (BlockerCount == 0.0f) {
        return 1.0f;
    }

    float BlockerDepth = {BlockerSum / BlockerCount};
    float BlockerDistance = {Perspective ? NearPlane * FarPlane / (FarPlane - BlockerDepth * (FarPlane - NearPlane)) : BlockerDepth * FarPlane};
    float PenumbraTile = {Perspective ? Map.mSoftness.x * (ReceiverDistance - BlockerDistance) / max(BlockerDistance, 0.0001f) * Map.mSoftness.y / ReceiverDistance : Map.mSoftness.x * (ReceiverDistance - BlockerDistance) * Map.mSoftness.y};
    float Radius = {clamp(PenumbraTile * Map.mAtlasRectangle.z, Texel, MaximumRadius)};

    return FilterPoissonShadow(AtlasUV, ReceiverDepth, Radius, Rotation, MinimumUV, MaximumUV);
}

/**
 * @brief 선택한 필터로 그림자 맵 하나를 샘플링합니다.
 * @param[in] Index 그림자 맵 구조화 버퍼의 0 기반 인덱스입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] PixelPosition PCSS 탭 회전에 쓰는 화면 픽셀 위치입니다.
 * @param[in] FilterScale PCF 탭 간격의 배율이며 캐스케이드 전환 구간에서 다음 맵의 월드 필터 폭으로 맞출 때 1보다 커집니다.
 * @return 0은 완전한 그림자, 1은 완전한 가시 상태인 계수이며 맵 밖은 1입니다.
 */
float SampleShadowMap(uint Index, float3 Position, float2 PixelPosition, float FilterScale) {
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

    float2 AtlasUV = {Map.mAtlasRectangle.xy + UV * Map.mAtlasRectangle.zw};
    float Texel = {Map.mInverseAtlasSize};
    float2 MinimumUV = {Map.mAtlasRectangle.xy + Texel * 0.5f};
    float2 MaximumUV = {Map.mAtlasRectangle.xy + Map.mAtlasRectangle.zw - Texel * 0.5f};
    uint Filter = {(uint)ShadowFilter.x};

    if (Filter == SHADOW_FILTER_HARD) {
        return Projected.z <= ShadowAtlas.Load(int3(clamp(AtlasUV, MinimumUV, MaximumUV) / Texel, 0)) ? 1.0f : 0.0f;
    }

    float Visibility = {0.0f};

    if (Filter == SHADOW_FILTER_PCSS) {
        Visibility = FilterPCSS(AtlasUV, Projected.z, PixelPosition, Map, MinimumUV, MaximumUV);
    } else {
        int Radius = {(int)ShadowFilter.y};

        [loop]
        for (int Y = {-Radius}; Y <= Radius; ++Y) {
            [loop]
            for (int X = {-Radius}; X <= Radius; ++X) {
                float2 Offset = {float2(X, Y) * Texel * FilterScale};
                Visibility += ShadowAtlas.SampleCmpLevelZero(ShadowSampler, clamp(AtlasUV + Offset, MinimumUV, MaximumUV), Projected.z);
            }
        }

        Visibility /= (float)((2 * Radius + 1) * (2 * Radius + 1));
    }

    return saturate((Visibility - 0.5f) * (1.0f + Map.mSharpen) + 0.5f);
}

/**
 * @brief 광원 종류에 맞는 그림자 면·캐스케이드를 선택하고 전환 구간을 혼합합니다.
 * @param[in] Light 평가할 광원의 GPU 정보입니다.
 * @param[in] Position 조명을 평가할 월드 공간 위치입니다.
 * @param[in] PixelPosition PCSS 탭 회전에 쓰는 화면 픽셀 위치입니다.
 * @return 광원의 그림자 가시성 계수이며 그림자가 없으면 1입니다.
 */
float ShadowVisibility(FLight Light, float3 Position, float2 PixelPosition) {
    if (Light.mShadowInfo.x < 1.0f) {
        return 1.0f;
    }

    uint Start = {(uint)Light.mShadowInfo.x - 1};
    uint Count = {(uint)Light.mShadowInfo.y};
    uint Type = {(uint)Light.mDirectionAndType.w};

    if (Type == 1) {
        float Depth = {dot(Position - ShadowCamera.xyz, ShadowForward.xyz)};

        for (uint Cascade = {0}; Cascade < Count; ++Cascade) {
            FShadowMap Map = {ShadowMaps[Start + Cascade]};

            if (Depth <= Map.mCascade.y) {
                if (Depth <= Map.mCascade.z || Map.mCascade.y <= Map.mCascade.z) {
                    return SampleShadowMap(Start + Cascade, Position, PixelPosition, 1.0f);
                }

                float Blend = {smoothstep(Map.mCascade.z, Map.mCascade.y, Depth)};

                if (Cascade + 1 >= Count) {
                    return lerp(SampleShadowMap(Start + Cascade, Position, PixelPosition, 1.0f), 1.0f, Blend);
                }

                // 값만 섞으면 맵마다 다른 월드 필터 폭이 전환 구간에서 급변하므로, 현재 맵의 필터 폭도 다음 맵의 폭까지 함께 넓힙니다.
                FShadowMap NextMap = {ShadowMaps[Start + Cascade + 1]};
                float TexelRatio = {clamp((Map.mSoftness.y * Map.mAtlasRectangle.z) / (NextMap.mSoftness.y * NextMap.mAtlasRectangle.z), 1.0f, 4.0f)};
                float Current = {SampleShadowMap(Start + Cascade, Position, PixelPosition, lerp(1.0f, TexelRatio, Blend))};
                float Next = {SampleShadowMap(Start + Cascade + 1, Position, PixelPosition, 1.0f)};

                return lerp(Current, Next, Blend);
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

    return SampleShadowMap(Start + Face, Position, PixelPosition, 1.0f);
}
