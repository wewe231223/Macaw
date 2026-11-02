/**
 * @file Sky.hlsl
 * @brief 등장방형 파노라마를 메시 없이 전체 화면 삼각형으로 그리는 기본 하늘 셰이더입니다.
 */

/**
 * @brief 픽셀의 화면 좌표를 시선 방향으로 복원하는 상수입니다.
 */
cbuffer SkyConstants : register(b0) {
    /**
     * @brief 이동을 제거한 뷰 행렬과 투영 행렬을 곱한 행 우선 역행렬입니다.
     */
    row_major float4x4 InverseViewRotationProjection;
};

/**
 * @brief 세로가 위에서 아래로 천정부터 천저까지 대응하는 등장방형 파노라마입니다.
 */
Texture2D<float4> Panorama : register(t0);

/**
 * @brief 가로는 반복하고 세로는 극점에서 고정하는 선형 샘플러입니다.
 */
SamplerState PanoramaSampler : register(s0);

/**
 * @brief 전체 화면 삼각형에서 픽셀 단계로 전달하는 클립 위치와 NDC 좌표입니다.
 */
struct FVertexOutput {
    /**
     * @brief 깊이를 원평면에 고정한 클립 위치입니다.
     */
    float4 mPosition : SV_POSITION;

    /**
     * @brief 시선 방향을 복원할 NDC XY 좌표입니다.
     */
    float2 mDeviceCoordinate : TEXCOORD0;
};

/**
 * @brief 정점 버퍼 없이 화면을 덮는 큰 삼각형을 원평면에 생성합니다.
 * @param[in] VertexId Draw(3, 0)에서 전달하는 0~2 정점 번호입니다.
 * @return 래스터라이저에 전달할 클립 위치와 NDC 좌표입니다.
 */
FVertexOutput VertexMain(uint VertexId : SV_VertexID) {
    FVertexOutput Output = {(FVertexOutput)0};
    Output.mDeviceCoordinate = float2((VertexId << 1) & 2, VertexId & 2) * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    Output.mPosition = float4(Output.mDeviceCoordinate, 1.0f, 1.0f);

    return Output;
}

/**
 * @brief 시선 방향을 파노라마 좌표로 변환해 표본을 출력합니다.
 * @param[in] Input 보간된 NDC 좌표입니다.
 * @return 파노라마 RGB와 불투명 알파입니다.
 * @note U는 atan2 이음매에서 불연속이므로 밉 선택에는 연속인 V의 미분만 사용합니다.
 */
float4 PixelMain(FVertexOutput Input) : SV_TARGET {
    static const float Pi = 3.14159265f;

    float4 FarPoint = {mul(float4(Input.mDeviceCoordinate, 1.0f, 1.0f), InverseViewRotationProjection)};
    float3 Direction = {normalize(FarPoint.xyz / FarPoint.w)};
    float U = {atan2(-Direction.y, Direction.x) / (2.0f * Pi)};
    float V = {acos(clamp(Direction.z, -1.0f, 1.0f)) / Pi};
    float3 Color = {Panorama.SampleGrad(PanoramaSampler, float2(U, V), float2(0.0f, ddx(V)), float2(0.0f, ddy(V))).rgb};

    return float4(Color, 1.0f);
}
