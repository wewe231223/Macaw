/**
 * @file MaterialCommon.hlsli
 * @brief 메시 재질의 객체·표면 상수와 정점 입출력 형식을 정의합니다.
 */

/**
 * @brief 메시 객체의 월드·역월드 행렬과 UV·그림자 플래그 상수입니다.
 */
cbuffer ObjectConstants : register(b0) {
    /**
     * @brief 객체 로컬 좌표를 월드 공간으로 변환하는 행 우선 행렬입니다.
     */
    row_major float4x4 World;
    /**
     * @brief 월드 행렬의 역행렬이며 법선의 역전치 변환에 사용합니다.
     */
    row_major float4x4 WorldInverse;
    /**
     * @brief 텍스처 UV의 배율 XY와 오프셋 ZW입니다.
     */
    float4 UVTransform;
    /**
     * @brief 그림자 수신 여부와 예약값을 저장합니다.
     */
    float4 ShadowFlags;
};

#include "ViewConstants.hlsli"

/**
 * @brief 재질의 기본색과 표면 조명 계수 상수입니다.
 */
cbuffer MaterialConstants : register(b2) {
    /**
     * @brief 재질의 기본 RGBA 색상입니다.
     */
    float4 BaseColor;
    /**
     * @brief 정반사 강도·정반사 지수·노멀 맵 강도와 예약값입니다.
     */
    float4 SurfaceParameters;
};

/**
 * @brief 재질의 기본색 텍스처입니다.
 */
Texture2D BaseColorTexture : register(t0);
#ifdef HAS_NORMAL_MAP
/**
 * @brief 선형 색상으로 읽을 접선 공간 노멀 맵입니다.
 */
Texture2D NormalMapTexture : register(t1);
#endif
/**
 * @brief 재질 텍스처를 읽는 샘플러입니다.
 */
SamplerState MaterialSampler : register(s0);

/**
 * @brief 정점 셰이더가 읽을 메시 또는 선분 입력 형식입니다.
 */
struct FInput {
    /**
     * @brief 정점의 로컬 공간 위치입니다.
     */
    float3 mPosition : POSITION;
    /**
     * @brief 법선 방향이며 정점 입력은 로컬 공간, 픽셀 입력은 월드 공간입니다.
     */
    float3 mNormal : NORMAL;
    /**
     * @brief 접선 XYZ와 종법선 방향 부호 W이며 입력 단계에 따라 로컬·월드 공간을 사용합니다.
     */
    float4 mTangent : TANGENT;
    /**
     * @brief 배율·오프셋을 적용할 텍스처 좌표입니다.
     */
    float2 mUV : TEXCOORD0;
    /**
     * @brief 정점에서 전달하는 RGBA 색상입니다.
     */
    float4 mColor : COLOR;
#ifdef MacawInstanced
    /**
     * @brief 인스턴스 월드 행렬의 첫 번째 행입니다.
     */
    float4 mWorld0 : INSTANCE_WORLD0;
    /**
     * @brief 인스턴스 월드 행렬의 두 번째 행입니다.
     */
    float4 mWorld1 : INSTANCE_WORLD1;
    /**
     * @brief 인스턴스 월드 행렬의 세 번째 행입니다.
     */
    float4 mWorld2 : INSTANCE_WORLD2;
    /**
     * @brief 인스턴스 월드 행렬의 네 번째 행입니다.
     */
    float4 mWorld3 : INSTANCE_WORLD3;
    /**
     * @brief 인스턴스 역월드 행렬의 첫 번째 행입니다.
     */
    float4 mInverse0 : INSTANCE_INVERSE0;
    /**
     * @brief 인스턴스 역월드 행렬의 두 번째 행입니다.
     */
    float4 mInverse1 : INSTANCE_INVERSE1;
    /**
     * @brief 인스턴스 역월드 행렬의 세 번째 행입니다.
     */
    float4 mInverse2 : INSTANCE_INVERSE2;
    /**
     * @brief 인스턴스 역월드 행렬의 네 번째 행입니다.
     */
    float4 mInverse3 : INSTANCE_INVERSE3;
    /**
     * @brief 인스턴스의 UV 배율 XY와 오프셋 ZW입니다.
     */
    float4 mUVTransform : INSTANCE_UV;
    /**
     * @brief 인스턴스의 그림자 수신 여부와 예약값입니다.
     */
    float4 mShadowFlags : INSTANCE_SHADOW;
#endif
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
     * @brief 법선 방향이며 정점 입력은 로컬 공간, 픽셀 입력은 월드 공간입니다.
     */
    float3 mNormal : NORMAL;
    /**
     * @brief 접선 XYZ와 종법선 방향 부호 W이며 입력 단계에 따라 로컬·월드 공간을 사용합니다.
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
    /**
     * @brief 프리미티브의 그림자 수신 여부이며 보간하지 않습니다.
     */
    nointerpolation float mReceiveShadow : TEXCOORD4;
};
