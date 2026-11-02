/**
 * @file MaterialCommon.hlsli
 * @brief 메시 재질의 표면 상수와 텍스처 및 공통 픽셀 입력을 정의합니다.
 */

#ifndef MacawMaterialCommonIncluded
#define MacawMaterialCommonIncluded

#include "PixelInput.hlsli"

/**
 * @brief 재질의 기본색과 표면 조명 계수 상수입니다.
 */
cbuffer MaterialConstants : register(b2) {
    /**
     * @brief 재질의 기본 RGBA 색상입니다.
     */
    float4 BaseColor;

    /**
     * @brief 정반사 강도·정반사 지수·노멀 맵 강도·환경 반사 강도입니다.
     */
    float4 SurfaceParameters;

    float4 SpecularColor;
    float4 AmbientColor;
    float4 EmissiveColor;
    float4 ReflectionColor;
    float4 TextureFlags;
};

Texture2D SpecularTexture : register(t7);
Texture2D EmissiveTexture : register(t8);
Texture2D ReflectionTexture : register(t9);

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

#endif
