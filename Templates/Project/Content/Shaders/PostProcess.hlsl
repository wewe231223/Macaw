/**
 * @file PostProcess.hlsl
 * @brief 장면 컬러와 깊이로 높이 안개·깊이 시각화·FXAA를 처리하는 전체 화면 후처리 셰이더입니다.
 */

/**
 * @brief FPostProcessConstants와 같은 224바이트 레이아웃으로 픽셀 셰이더 b0에 전달하는 상수입니다.
 */
cbuffer PostProcessConstants : register(b0) {
    /**
     * @brief 장치 깊이와 화면 좌표를 월드 위치로 복원하는 행 우선 역뷰투영 행렬입니다.
     */
    row_major float4x4 InverseViewProjection;

    /**
     * @brief 복원한 월드 위치를 카메라 공간의 선형 깊이로 변환하는 행 우선 뷰 행렬입니다.
     */
    row_major float4x4 View;

    /**
     * @brief 카메라 월드 위치 XYZ와 정사영 여부 W입니다.
     */
    float4 CameraPosition;

    /**
     * @brief 출력 폭·높이 XY와 각각의 역수 ZW입니다.
     */
    float4 ScreenSize;

    /**
     * @brief 기본 산란에 사용할 안개 RGBA 색상이며 합성에는 RGB를 사용합니다.
     */
    float4 FogColor;

    /**
     * @brief 기준 밀도·높이 감쇠·기준 월드 Z·최대 불투명도입니다.
     */
    float4 FogParameters;

    /**
     * @brief 안개 시작 거리·적분 종료 거리·적용 제외 거리와 예약값입니다.
     */
    float4 FogDistances;

    /**
     * @brief X는 복사 0·안개 1·깊이 2·FXAA 3 패스 선택값이며 나머지는 예약합니다.
     */
    float4 Settings;
};

/**
 * @brief 이전 장면 또는 안개 합성 패스의 입력 RGBA 컬러입니다.
 */
Texture2D<float4> SceneColor : register(t0);

/**
 * @brief D3D 0~1 범위의 장치 깊이를 기록한 장면 깊이 텍스처입니다.
 */
Texture2D<float> SceneDepth : register(t1);

/**
 * @brief 입력 컬러와 FXAA 주변 표본에 사용하는 선형 필터·Clamp 샘플러입니다.
 */
SamplerState LinearClamp : register(s0);

/**
 * @brief 전체 화면 삼각형에서 픽셀 단계로 전달하는 클립 위치와 UV입니다.
 */
struct FVertexOutput {
    /**
     * @brief 정점 단계의 클립 위치이며 픽셀 단계에서는 화면 픽셀 좌표입니다.
     */
    float4 mPosition : SV_POSITION;

    /**
     * @brief 장면 텍스처를 읽을 화면 UV이며 보이는 영역에서는 0~1 범위입니다.
     */
    float2 mUV : TEXCOORD0;
};

/**
 * @brief 정점 버퍼 없이 화면을 덮는 큰 삼각형과 대응 UV를 생성합니다.
 * @param[in] VertexId Draw(3, 0)에서 전달하는 0~2 정점 번호입니다.
 * @return 래스터라이저에 전달할 클립 위치와 UV입니다.
 */
FVertexOutput VertexMain(uint VertexId : SV_VertexID) {
    FVertexOutput Output = {(FVertexOutput)0};
    Output.mUV = float2((VertexId << 1) & 2, VertexId & 2);
    Output.mPosition = float4(Output.mUV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return Output;
}

/**
 * @brief 화면 UV와 장치 깊이를 역뷰투영한 뒤 동차 나눗셈으로 월드 위치를 복원합니다.
 * @param[in] UV 위쪽이 0인 장면 텍스처 좌표입니다.
 * @param[in] Depth D3D 0~1 범위의 장치 깊이입니다.
 * @return 해당 픽셀의 월드 공간 위치입니다.
 */
float3 ReconstructWorld(float2 UV, float Depth) {
    float4 World = {mul(float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), Depth, 1.0f), InverseViewProjection)};

    return World.xyz / World.w;
}

/**
 * @brief 광선 구간의 지수 높이 밀도를 적분하여 투과율과 기본 산란 색상을 장면 컬러에 합성합니다.
 * @param[in] UV 현재 픽셀의 장면 텍스처 좌표입니다.
 * @param[in] Color 이전 패스에서 읽은 입력 RGBA 색상입니다.
 * @param[in] Depth 현재 픽셀의 D3D 장치 깊이입니다.
 * @return 안개를 합성한 RGB와 원본 알파이며 적용 제외 거리에서는 원본을 반환합니다.
 * @note 정사영 광선은 카메라 평면에서 시작합니다. 높이 변화가 작을 때는 적분의 급수 근사를 사용합니다.
 */
float4 ApplyFog(float2 UV, float4 Color, float Depth) {
    float3 World = {ReconstructWorld(UV, min(Depth, 0.999999f))};
    float3 Origin = {CameraPosition.xyz};

    if (CameraPosition.w > 0.5f) {
        Origin = ReconstructWorld(UV, 0.0f);
        Origin -= float3(View[0][2], View[1][2], View[2][2]) * mul(float4(Origin, 1.0f), View).z;
    }

    float3 Ray = {World - Origin};
    float Distance = {length(Ray)};

    if (Distance <= FogDistances.x || (FogDistances.z > 0.0f && Distance > FogDistances.z)) {
        return Color;
    }

    float EndDistance = {FogDistances.y > 0.0f ? min(Distance, FogDistances.y) : Distance};
    float SegmentLength = {max(EndDistance - FogDistances.x, 0.0f)};
    float DirectionZ = {Ray.z / max(Distance, 0.000001f)};
    float StartHeight = {Origin.z + DirectionZ * FogDistances.x};
    float EndHeight = {StartHeight + DirectionZ * SegmentLength};
    float LowerExponent = {clamp(-FogParameters.y * (min(StartHeight, EndHeight) - FogParameters.z), -80.0f, 40.0f)};
    float HeightDelta = {FogParameters.y * abs(EndHeight - StartHeight)};
    float Integral = {HeightDelta < 0.001f ? 1.0f - 0.5f * HeightDelta + HeightDelta * HeightDelta / 6.0f : (1.0f - exp(-HeightDelta)) / HeightDelta};
    float OpticalDepth = {FogParameters.x * exp(LowerExponent) * SegmentLength * Integral};
    float Opacity = {min(1.0f - exp(-OpticalDepth), FogParameters.w)};

    return float4(lerp(Color.rgb, FogColor.rgb, saturate(Opacity)), Color.a);
}

/**
 * @brief RGB 가중합으로 FXAA의 가장자리 판정에 사용할 휘도를 계산합니다.
 * @param[in] Color 휘도로 환산할 입력 RGB 색상입니다.
 * @return 0.299·0.587·0.114 가중치로 계산한 휘도입니다.
 */
float Luminance(float3 Color) {
    return dot(Color, float3(0.299f, 0.587f, 0.114f));
}

/**
 * @brief 대각선 이웃의 휘도로 가장자리 방향을 구해 제한된 방향 필터로 계단 현상을 완화합니다.
 * @param[in] UV 필터를 적용할 장면 텍스처 좌표입니다.
 * @return 대비가 낮으면 원본, 가장자리이면 필터 결과와 원본 알파를 반환합니다.
 */
float4 ApplyFXAA(float2 UV) {
    float4 Center = {SceneColor.SampleLevel(LinearClamp, UV, 0.0f)};
    float3 NorthWest = {SceneColor.SampleLevel(LinearClamp, UV + float2(-1.0f, -1.0f) * ScreenSize.zw, 0.0f).rgb};
    float3 NorthEast = {SceneColor.SampleLevel(LinearClamp, UV + float2(1.0f, -1.0f) * ScreenSize.zw, 0.0f).rgb};
    float3 SouthWest = {SceneColor.SampleLevel(LinearClamp, UV + float2(-1.0f, 1.0f) * ScreenSize.zw, 0.0f).rgb};
    float3 SouthEast = {SceneColor.SampleLevel(LinearClamp, UV + float2(1.0f, 1.0f) * ScreenSize.zw, 0.0f).rgb};
    float LumaCenter = {Luminance(Center.rgb)};
    float LumaNorthWest = {Luminance(NorthWest)};
    float LumaNorthEast = {Luminance(NorthEast)};
    float LumaSouthWest = {Luminance(SouthWest)};
    float LumaSouthEast = {Luminance(SouthEast)};
    float LumaMinimum = {min(LumaCenter, min(min(LumaNorthWest, LumaNorthEast), min(LumaSouthWest, LumaSouthEast)))};
    float LumaMaximum = {max(LumaCenter, max(max(LumaNorthWest, LumaNorthEast), max(LumaSouthWest, LumaSouthEast)))};

    if (LumaMaximum - LumaMinimum < max(0.0312f, LumaMaximum * 0.125f)) {
        return Center;
    }

    float2 Direction = {float2(-((LumaNorthWest + LumaNorthEast) - (LumaSouthWest + LumaSouthEast)), (LumaNorthWest + LumaSouthWest) - (LumaNorthEast + LumaSouthEast))};
    float Reduction = {max((LumaNorthWest + LumaNorthEast + LumaSouthWest + LumaSouthEast) * (0.25f * 0.125f), 1.0f / 128.0f)};
    float Reciprocal = {rcp(min(abs(Direction.x), abs(Direction.y)) + Reduction)};
    Direction = clamp(Direction * Reciprocal, -8.0f, 8.0f) * ScreenSize.zw;
    float3 First = {0.5f * (SceneColor.SampleLevel(LinearClamp, UV + Direction * (-1.0f / 6.0f), 0.0f).rgb + SceneColor.SampleLevel(LinearClamp, UV + Direction * (1.0f / 6.0f), 0.0f).rgb)};
    float3 Second = {First * 0.5f + 0.25f * (SceneColor.SampleLevel(LinearClamp, UV + Direction * -0.5f, 0.0f).rgb + SceneColor.SampleLevel(LinearClamp, UV + Direction * 0.5f, 0.0f).rgb)};
    float LumaSecond = {Luminance(Second)};

    return float4(LumaSecond < LumaMinimum || LumaSecond > LumaMaximum ? First : Second, Center.a);
}

/**
 * @brief Settings.x에 따라 컬러 복사·안개 합성·선형 깊이 시각화·FXAA 중 한 패스를 실행합니다.
 * @param[in] Input 화면 픽셀 위치와 보간된 장면 UV입니다.
 * @return 최종 출력 타깃에 기록할 RGBA 색상입니다.
 * @note 깊이 시각화는 뷰 공간 Z에 1-exp(-Z*0.01)을 적용하며 깊이 1의 빈 배경은 흰색입니다.
 */
float4 PixelMain(FVertexOutput Input) : SV_TARGET {
    float4 Color = {SceneColor.SampleLevel(LinearClamp, Input.mUV, 0.0f)};
    float Depth = {SceneDepth.Load(int3(int2(Input.mPosition.xy), 0))};

    if (Settings.x == 1.0f) {
        return ApplyFog(Input.mUV, Color, Depth);
    }

    if (Settings.x == 2.0f) {
        float3 World = {ReconstructWorld(Input.mUV, min(Depth, 0.999999f))};
        float ViewDepth = {max(mul(float4(World, 1.0f), View).z, 0.0f)};
        float Value = {Depth >= 1.0f ? 1.0f : 1.0f - exp(-ViewDepth * 0.01f)};

        return float4(Value, Value, Value, 1.0f);
    }

    if (Settings.x == 3.0f) {
        return ApplyFXAA(Input.mUV);
    }

    return Color;
}
