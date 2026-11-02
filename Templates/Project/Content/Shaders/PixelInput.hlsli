#ifndef MacawPixelInputIncluded
#define MacawPixelInputIncluded

/**
 * @brief 래스터라이저를 거쳐 픽셀 셰이더로 전달할 정점 출력입니다.
 */
struct FPixelInput {
    /**
     * @brief 래스터라이저에 전달하거나 픽셀 단계에서 읽는 투영 위치입니다.
     */
    float4 mPosition : SV_POSITION;

    /**
     * @brief 월드 공간 표면 법선입니다.
     */
    float3 mNormal : NORMAL;

    /**
     * @brief 월드 공간 접선 XYZ와 종법선 방향 부호 W입니다.
     */
    float4 mTangent : TANGENT;

    /**
     * @brief 배율·오프셋을 적용할 텍스처 좌표입니다.
     */
    float2 mUV : TEXCOORD0;

    /**
     * @brief 변환된 정점의 월드 공간 위치입니다.
     */
    float3 mWorldPosition : TEXCOORD1;

    /**
     * @brief 광원 계산으로 누적한 RGB 확산 조명입니다.
     */
    float3 mDiffuse : TEXCOORD2;

    /**
     * @brief 광원 계산으로 누적한 RGB 정반사 조명입니다.
     */
    float3 mSpecular : TEXCOORD3;

    /**
     * @brief 정점에서 전달하는 RGBA 색상입니다.
     */
    float4 mColor : COLOR;

    float3 mAmbient : TEXCOORD4;
};

#endif
