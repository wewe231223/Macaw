/**
 * @file ViewConstants.hlsli
 * @brief 월드 좌표를 클립 공간으로 변환하는 공통 뷰 상수입니다.
 */

#ifndef MacawViewConstants
#define MacawViewConstants

/**
 * @brief 현재 카메라의 공통 뷰·투영 변환 상수입니다.
 */
cbuffer ViewConstants : register(b1) {
    /**
     * @brief 월드 위치를 카메라 클립 공간으로 변환하는 행 우선 행렬입니다.
     */
    row_major float4x4 ViewProjection;
};

#endif
