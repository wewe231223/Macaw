/**
 * @file Depth.hlsl
 * @brief 그림자 아틀라스에 메시 깊이만 기록하는 깊이 전용 패스 셰이더입니다.
 */

#include "ViewConstants.hlsli"

/**
 * @brief 비인스턴스 드로에서 사용하는 메시 객체의 월드 행렬 상수입니다.
 */
cbuffer ObjectConstants : register(b0) {
    /**
     * @brief 객체 로컬 좌표를 월드 공간으로 변환하는 행 우선 행렬입니다.
     */
    row_major float4x4 World;
};

/**
 * @brief 깊이 패스 정점 셰이더가 읽는 위치와 인스턴스 월드 행렬 입력입니다.
 */
struct FDepthInput {
    /**
     * @brief 정점의 로컬 공간 위치입니다.
     */
    float3 mPosition : POSITION;

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
#endif
};

/**
 * @brief 로컬 위치를 현재 깊이 패스의 클립 공간으로 변환합니다.
 * @param Input 정점 위치와 인스턴스 월드 행렬입니다.
 * @return 깊이 패스 클립 공간 위치입니다.
 */
float4 VertexMain(FDepthInput Input) : SV_POSITION {
#ifdef MacawInstanced
    row_major float4x4 ObjectWorld = {float4x4(Input.mWorld0, Input.mWorld1, Input.mWorld2, Input.mWorld3)};
#else
    row_major float4x4 ObjectWorld = {World};
#endif

    return mul(mul(float4(Input.mPosition, 1.0f), ObjectWorld), ViewProjection);
}

/**
 * @brief 뷰포트 전체를 덮는 삼각형을 최대 깊이에 그려 시저 영역의 깊이만 지웁니다.
 * @param Index 0~2의 정점 번호입니다.
 * @return 깊이가 1인 클립 공간 위치입니다.
 */
float4 ClearMain(uint Index : SV_VertexID) : SV_POSITION {
    const float2 UV = float2((Index << 1) & 2, Index & 2);

    return float4(UV * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 1.0f, 1.0f);
}
