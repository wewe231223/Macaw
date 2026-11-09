/**
 * @file Grid.hlsl
 * @brief 카메라를 따라다니는 Z=0 평면 사각형에 에디터 그리드 선을 픽셀 단위로 계산해 그립니다.
 */

#pragma pack_matrix(row_major)

/**
 * @brief 그리드 평면의 변환과 선 간격 상수입니다.
 */
cbuffer GridConstants : register(b0) {
    /**
     * @brief 월드 위치를 클립 공간으로 변환하는 행 우선 행렬입니다.
     */
    float4x4 ViewProjection;

    /**
     * @brief 카메라 월드 XY, 보조선 간격 Z와 평면 반지름 W입니다.
     */
    float4 CameraSpacingRadius;
};

/**
 * @brief 래스터라이저를 거쳐 픽셀 셰이더로 전달할 정점 출력입니다.
 */
struct FOutput {
    /**
     * @brief 래스터라이저에 전달하는 투영 위치입니다.
     */
    float4 mPosition : SV_POSITION;

    /**
     * @brief 카메라 바로 아래 지점에서의 평면 위 XY 거리입니다.
     */
    float2 mOffset : TEXCOORD0;
};

/**
 * @brief 정점 ID로 카메라 중심의 Z=0 평면 사각형을 생성합니다.
 * @param[in] Vertex 생성할 사각형 삼각형 목록의 0~5 정점 번호입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FOutput VertexMain(uint Vertex : SV_VertexID) {
    const float2 Corners[6] = {float2(-1.0f, 1.0f), float2(-1.0f, -1.0f), float2(1.0f, 1.0f), float2(1.0f, 1.0f), float2(-1.0f, -1.0f), float2(1.0f, -1.0f)};
    FOutput Output = (FOutput)0;
    Output.mOffset = Corners[Vertex] * CameraSpacingRadius.w;
    Output.mPosition = mul(float4(CameraSpacingRadius.xy + Output.mOffset, 0.0f, 1.0f), ViewProjection);

    return Output;
}

/**
 * @brief 한 축의 선 피복률을 픽셀 거리로 계산하고 칸이 픽셀보다 촘촘해지기 전에 지웁니다.
 * @param[in] Value 평면 위 월드 좌표입니다.
 * @param[in] Gradient 픽셀당 월드 좌표 변화량입니다.
 * @param[in] Spacing 선 간격입니다.
 * @return 0~1 범위의 선 불투명도입니다.
 */
float AxisCoverage(float Value, float Gradient, float Spacing) {
    float CellPixels = Spacing / max(Gradient, 0.000001f);
    float EdgeDistance = abs(frac(Value / Spacing + 0.5f) - 0.5f) * CellPixels;

    return saturate(1.0f - EdgeDistance) * saturate((CellPixels - 4.0f) / 8.0f);
}

/**
 * @brief 보조선과 10칸 주선을 합성하고 평면 가장자리로 갈수록 투명하게 출력합니다.
 * @param[in] Input 현재 픽셀의 평면 위 거리입니다.
 * @return 출력 타깃에 혼합할 RGBA 색상입니다.
 */
float4 PixelMain(FOutput Input) : SV_TARGET {
    const float3 MinorColor = float3(0.13f, 0.15f, 0.19f);
    const float3 MajorColor = float3(0.22f, 0.25f, 0.30f);
    float2 World = CameraSpacingRadius.xy + Input.mOffset;
    float2 Gradient = float2(length(float2(ddx(Input.mOffset.x), ddy(Input.mOffset.x))), length(float2(ddx(Input.mOffset.y), ddy(Input.mOffset.y))));
    float Spacing = CameraSpacingRadius.z;
    float Minor = max(AxisCoverage(World.x, Gradient.x, Spacing), AxisCoverage(World.y, Gradient.y, Spacing));
    float Major = max(AxisCoverage(World.x, Gradient.x, Spacing * 10.0f), AxisCoverage(World.y, Gradient.y, Spacing * 10.0f));
    float Coverage = max(Minor, Major);
    float Alpha = Coverage * (1.0f - smoothstep(0.5f, 1.0f, length(Input.mOffset) / CameraSpacingRadius.w));
    clip(Alpha - 0.001f);

    return float4(lerp(MinorColor, MajorColor, Major / max(Coverage, 0.001f)), Alpha);
}
