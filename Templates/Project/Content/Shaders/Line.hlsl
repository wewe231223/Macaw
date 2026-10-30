/**
 * @file Line.hlsl
 * @brief 월드 선분을 픽셀 두께의 사각형으로 확장하고 가장자리를 부드럽게 그립니다.
 */

#include "ViewConstants.hlsli"

/**
 * @brief 픽셀 두께의 선분 확장에 사용하는 출력 크기 상수입니다.
 */
cbuffer LineConstants : register(b0) {
    /**
     * @brief 출력 폭·높이와 각각의 역수를 픽셀 단위로 저장합니다.
     */
    float4 Viewport;
};

/**
 * @brief 정점 셰이더가 읽을 메시 또는 선분 입력 형식입니다.
 */
struct FInput {
    /**
     * @brief 선분의 시작 월드 위치 XYZ와 픽셀 두께 W입니다.
     */
    float4 mStartAndWidth : START;
    /**
     * @brief 선분의 끝 월드 위치 XYZ와 예약값입니다.
     */
    float4 mEnd : END;
    /**
     * @brief 정점에서 전달하는 RGBA 색상입니다.
     */
    float4 mColor : COLOR;
};

/**
 * @brief 래스터라이저를 거쳐 픽셀 셰이더로 전달할 정점 출력입니다.
 */
struct FOutput {
    /**
     * @brief 래스터라이저에 전달하거나 픽셀 단계에서 읽는 투영 위치입니다.
     */
    float4 mPosition : SV_POSITION;
    /**
     * @brief 정점에서 전달하는 RGBA 색상입니다.
     */
    float4 mColor : COLOR;
    /**
     * @brief 선 중심에서의 화면상 거리이며 원근 보정을 적용하지 않습니다.
     */
    noperspective float mEdgeDistance : TEXCOORD0;
    /**
     * @brief 선 두께의 절반을 픽셀 단위로 저장하며 보간하지 않습니다.
     */
    nointerpolation float mHalfWidth : TEXCOORD1;
};

/**
 * @brief 선분을 근평면에서 잘라 픽셀 두께의 사각형 정점으로 확장합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @param[in] Vertex 생성할 사각형 삼각형 목록의 0~5 정점 번호입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FOutput VertexMain(FInput Input, uint Vertex : SV_VertexID) {
    FOutput Output = (FOutput)0;
    float4 Start = mul(float4(Input.mStartAndWidth.xyz, 1.0f), ViewProjection);
    float4 End = mul(float4(Input.mEnd.xyz, 1.0f), ViewProjection);
    float StartDistance = Start.z - 0.00001f;
    float EndDistance = End.z - 0.00001f;
    if (StartDistance < 0.0f && EndDistance < 0.0f) {
        Output.mPosition = float4(0.0f, 0.0f, -1.0f, 1.0f);
        return Output;
    }
    if (StartDistance < 0.0f) {
        Start = lerp(Start, End, saturate(-StartDistance / (EndDistance - StartDistance)));
    } else if (EndDistance < 0.0f) {
        End = lerp(End, Start, saturate(-EndDistance / (StartDistance - EndDistance)));
    }
    float2 Direction = (End.xy / End.w - Start.xy / Start.w) * float2(Viewport.x, -Viewport.y);
    float Length = length(Direction);
    Direction = Length > 0.0001f ? Direction / Length : float2(1.0f, 0.0f);
    const float2 Corners[6] = {float2(0.0f, -1.0f), float2(0.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, -1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f)};
    float2 Corner = Corners[Vertex];
    float HalfWidth = max(Input.mStartAndWidth.w * 0.5f, 0.5f);
    float4 Position = Corner.x < 0.5f ? Start : End;
    Position.xy += float2(-Direction.y, Direction.x) * Corner.y * (HalfWidth + 0.5f) * float2(2.0f * Viewport.z, -2.0f * Viewport.w) * Position.w;
    Output.mPosition = Position;
    Output.mColor = Input.mColor;
    Output.mEdgeDistance = Corner.y * (HalfWidth + 0.5f);
    Output.mHalfWidth = HalfWidth;
    return Output;
}

/**
 * @brief 선 가장자리의 피복률을 계산하고 RGBA 색상을 출력합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain(FOutput Input) : SV_TARGET {
    float Coverage = saturate(Input.mHalfWidth + 0.5f - abs(Input.mEdgeDistance));
    clip(Input.mColor.a * Coverage - 0.001f);
    return float4(Input.mColor.rgb, Input.mColor.a * Coverage);
}
