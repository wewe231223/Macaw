/**
 * @file Text.hlsl
 * @brief MSDF 폰트 아틀라스로 글리프의 화면상 피복률을 계산하여 텍스트를 그립니다.
 */

#pragma pack_matrix(row_major)

/**
 * @brief 텍스트의 클립 변환·색상·MSDF 거리 상수입니다.
 */
cbuffer TextConstants : register(b0) {
    /**
     * @brief 로컬 평면 위치를 클립 공간으로 변환하는 행 우선 행렬입니다.
     */
    float4x4 Transform;
    /**
     * @brief 드로에 적용할 RGBA 색상입니다.
     */
    float4 Color;
    /**
     * @brief MSDF 거리 범위 X와 아틀라스 폭·높이 YZ, 예약값입니다.
     */
    float4 Parameters;
};

/**
 * @brief 글리프 정점의 로컬 평면 위치와 아틀라스 UV입니다.
 */
struct FVertexInput {
    /**
     * @brief 정점의 로컬 공간 위치입니다.
     */
    float2 Position : POSITION;
    /**
     * @brief 폰트 아틀라스 안의 글리프 텍스처 좌표입니다.
     */
    float2 UV : TEXCOORD0;
};

/**
 * @brief 글리프 픽셀의 화면 위치와 보간된 아틀라스 UV입니다.
 */
struct FPixelInput {
    /**
     * @brief 래스터라이저에 전달하거나 픽셀 단계에서 읽는 투영 위치입니다.
     */
    float4 Position : SV_POSITION;
    /**
     * @brief 폰트 아틀라스 안의 글리프 텍스처 좌표입니다.
     */
    float2 UV : TEXCOORD0;
};

/**
 * @brief RGB 채널에 부호 거리 정보를 저장한 MSDF 폰트 아틀라스입니다.
 */
Texture2D FontAtlas : register(t0);
/**
 * @brief 폰트 아틀라스의 거리 정보를 읽는 샘플러입니다.
 */
SamplerState FontSampler : register(s0);

/**
 * @brief 입력 정점의 클립 위치와 다음 단계의 보간 데이터를 계산합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FPixelInput VertexMain(FVertexInput Input) {
    FPixelInput Output = (FPixelInput)0;
    Output.Position = mul(float4(Input.Position, 0.0f, 1.0f), Transform);
    Output.UV = Input.UV;
    return Output;
}

/**
 * @brief MSDF 거리의 중앙값과 화면 미분으로 글리프의 색상·불투명도를 계산합니다.
 * @param[in] Input 현재 단계에 전달된 정점 또는 픽셀 입력입니다.
 * @return 출력 타깃에 기록할 RGBA 색상입니다.
 */
float4 PixelMain(FPixelInput Input) : SV_TARGET {
    float3 Distance = FontAtlas.Sample(FontSampler, Input.UV).rgb;
    float Median = max(min(Distance.r, Distance.g), min(max(Distance.r, Distance.g), Distance.b));
    float2 UnitRange = Parameters.xx / Parameters.yz;
    float2 ScreenSize = 1.0f / max(fwidth(Input.UV), float2(0.000001f, 0.000001f));
    float PixelRange = max(0.5f * dot(UnitRange, ScreenSize), 1.0f);
    float Opacity = saturate(PixelRange * (Median - 0.5f) + 0.5f);
    clip(Opacity * Color.a - 0.001f);
    return float4(Color.rgb, Color.a * Opacity);
}
