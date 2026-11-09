#include "LightingCommon.hlsli"

/**
 * @brief 메시 객체의 월드·역월드 행렬과 UV 상수입니다.
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
};

#include "ViewConstants.hlsli"

/**
 * @brief 정점 셰이더가 읽을 메시 또는 선분 입력 형식입니다.
 */
struct FMeshVertexInput {
#ifdef MacawSkinned
    /**
     * @brief 이 정점에 영향을 주는 본 인덱스 네 개입니다.
     */
    uint4 mBoneIndices : BLENDINDICES;

    /**
     * @brief 본 인덱스에 대응하는 가중치입니다.
     */
    float4 mBoneWeights : BLENDWEIGHT;
#endif
    /**
     * @brief 정점의 로컬 공간 위치입니다.
     */
    float3 mPosition : POSITION;

    /**
     * @brief 정점의 로컬 공간 법선입니다.
     */
    float3 mNormal : NORMAL;

    /**
     * @brief 로컬 공간 접선 XYZ와 종법선 방향 부호 W입니다.
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
#endif
};

/**
 * @brief 입력 정점의 클립 위치와 다음 단계의 보간 데이터를 계산합니다.
 * @param[in] Input 메시의 정점·인스턴스 입력입니다.
 * @return 래스터라이저에 전달할 정점 출력입니다.
 */
FPixelInput BuildPixelInput(FMeshVertexInput Input) {
    FPixelInput Output = {(FPixelInput)0};

#ifdef MacawInstanced
    row_major float4x4 ObjectWorld = {float4x4(Input.mWorld0, Input.mWorld1, Input.mWorld2, Input.mWorld3)};
    row_major float4x4 ObjectInverse = {float4x4(Input.mInverse0, Input.mInverse1, Input.mInverse2, Input.mInverse3)};
    float4 ObjectUV = {Input.mUVTransform};
#else
    row_major float4x4 ObjectWorld = {World};
    row_major float4x4 ObjectInverse = {WorldInverse};
    float4 ObjectUV = {UVTransform};
#endif

    Output.mWorldPosition = mul(float4(Input.mPosition, 1.0f), ObjectWorld).xyz;
    Output.mPosition = mul(float4(Output.mWorldPosition, 1.0f), ViewProjection);
    Output.mNormal = SafeNormal(mul(Input.mNormal, transpose((float3x3)ObjectInverse)));
    Output.mTangent = float4(SafeNormal(mul(Input.mTangent.xyz, (float3x3)ObjectWorld)), Input.mTangent.w * (determinant((float3x3)ObjectWorld) < 0.0f ? -1.0f : 1.0f));
    Output.mUV = Input.mUV * ObjectUV.xy + ObjectUV.zw;
    Output.mColor = Input.mColor;

#if defined(LIGHTING_MODEL_GOURAUD) && !defined(MACAW_UNLIT) && !defined(WORLD_NORMAL)
    float3 Normal = {Output.mNormal};

#ifdef HAS_NORMAL_MAP
    Normal = MappedNormal(Normal, Output.mTangent, NormalMapTexture.SampleLevel(MaterialSampler, Output.mUV, 0).xyz);
#endif

    for (uint Index = {0}; Index < (TileDimensions.w & 0x7fffffffu); ++Index) {
        FLighting Lighting = {EvaluateLight(Lights[Index], Output.mWorldPosition, Normal)};
        Output.mDiffuse += Lighting.mDiffuse;
        Output.mSpecular += Lighting.mSpecular;
        Output.mAmbient += Lighting.mAmbient;
    }
#endif

    return Output;
}

