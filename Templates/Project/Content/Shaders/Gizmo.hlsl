/**
 * @file Gizmo.hlsl
 * @brief 에디터 기즈모 메시를 지정한 단색으로 그립니다.
 */

/**
 * @brief 기즈모의 월드 변환과 단색 드로 상수입니다.
 */
cbuffer EditorMeshConstants : register(b0) {
    /**
     * @brief 객체 로컬 좌표를 월드 공간으로 변환하는 행 우선 행렬입니다.
     */
    row_major float4x4 World;
    /**
     * @brief 드로에 적용할 RGBA 색상입니다.
     */
    float4 Color;
};
#include "ViewConstants.hlsli"
/**
 * @brief 정점 셰이더가 읽을 메시 또는 선분 입력 형식입니다.
 */
struct FInput {
    /**
     * @brief 정점의 로컬 공간 위치입니다.
     */
    float3 mPosition : POSITION;
};
/**
 * @brief 래스터라이저를 거쳐 픽셀 셰이더로 전달할 정점 출력입니다.
 */
struct FOutput {
    /**
     * @brief 래스터라이저에 전달하거나 픽셀 단계에서 읽는 투영 위치입니다.
     */
    float4 mPosition : SV_POSITION;
};
/**
 * @brief 입력 정점의 클립 위치와 다음 단계의 보간 데이터를 계산합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FOutput VertexMain(FInput Input) {
    FOutput Output = {float4(0, 0, 0, 0)};
    Output.mPosition = mul(mul(float4(Input.mPosition, 1), World), ViewProjection);
    return Output;
}
/**
 * @brief 드로 상수 또는 텍스처의 RGBA 색상을 출력합니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain() : SV_TARGET {
    return Color;
}
