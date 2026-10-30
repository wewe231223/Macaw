/**
 * @file Sprite.hlsl
 * @brief 정점 ID로 빌보드 사각형을 생성하고 스프라이트 텍스처를 그립니다.
 */

#pragma pack_matrix(row_major)

/**
 * @brief 스프라이트의 클립 변환·UV·색상 상수입니다.
 */
cbuffer SpriteConstants : register(b0) {
    /**
     * @brief 로컬 평면 위치를 클립 공간으로 변환하는 행 우선 행렬입니다.
     */
    float4x4 Transform;
    /**
     * @brief 텍스처 UV의 배율 XY와 오프셋 ZW입니다.
     */
    float4 UVTransform;
    /**
     * @brief 드로에 적용할 RGBA 색상입니다.
     */
    float4 Color;
};

/**
 * @brief 스프라이트의 RGBA 텍스처입니다.
 */
Texture2D SpriteTexture : register(t0);
/**
 * @brief 스프라이트 텍스처를 읽는 샘플러입니다.
 */
SamplerState SpriteSampler : register(s0);

/**
 * @brief 래스터라이저를 거쳐 픽셀 셰이더로 전달할 정점 출력입니다.
 */
struct FOutput {
    /**
     * @brief 래스터라이저에 전달하거나 픽셀 단계에서 읽는 투영 위치입니다.
     */
    float4 mPosition : SV_POSITION;
    /**
     * @brief 배율·오프셋을 적용할 텍스처 좌표입니다.
     */
    float2 mUV : TEXCOORD0;
};

/**
 * @brief 정점 ID로 사각형을 생성하고 클립 위치와 변환된 UV를 계산합니다.
 * @param[in] Vertex 생성할 사각형 삼각형 목록의 0~5 정점 번호입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FOutput VertexMain(uint Vertex : SV_VertexID) {
    const float2 Corners[6] = {float2(-0.5f, 0.5f), float2(-0.5f, -0.5f), float2(0.5f, 0.5f), float2(0.5f, 0.5f), float2(-0.5f, -0.5f), float2(0.5f, -0.5f)};
    FOutput Output = (FOutput)0;
    Output.mPosition = mul(float4(Corners[Vertex], 0.0f, 1.0f), Transform);
    Output.mUV = (Corners[Vertex] * float2(1.0f, -1.0f) + 0.5f) * UVTransform.xy + UVTransform.zw;
    return Output;
}

/**
 * @brief 드로 상수 또는 텍스처의 RGBA 색상을 출력합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain(FOutput Input) : SV_TARGET {
    float4 Surface = SpriteTexture.Sample(SpriteSampler, Input.mUV) * Color;
    clip(Surface.a - 0.001f);
    return Surface;
}
