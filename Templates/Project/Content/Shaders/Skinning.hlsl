/**
 * @file Skinning.hlsl
 * @brief 베이스 패스와 그림자 패스가 공유하는 네 본 선형 스키닝입니다.
 */
#ifndef MacawSkinningIncluded
#define MacawSkinningIncluded

#ifdef MacawSkinned
/** @brief CPU에서 본마다 업로드하는 위치 행렬과 법선 역전치 행렬입니다. */
struct FSkinBone {
    /** @brief 바인드 위치를 현재 메시 공간 위치로 옮기는 행렬입니다. */
    row_major float4x4 mTransform;
    /** @brief 위치 행렬의 역전치로 계산한 법선 변환입니다. */
    row_major float4x4 mNormalTransform;
};

/** @brief 재질·조명과 독립된 정점 단계 슬롯의 객체별 본 팔레트입니다. */
StructuredBuffer<FSkinBone> SkinBones : register(t10);

/** @brief 정규화한 본 영향으로 바인드 위치를 현재 메시 공간으로 옮깁니다. @param Position 바인드 위치입니다. @param BoneIndices 본 번호입니다. @param Weights 본 영향입니다. @return 현재 포즈의 메시 공간 위치입니다. */
float3 SkinPosition(float3 Position, uint4 BoneIndices, float4 Weights) {
    const float Total = {dot(Weights, float4(1.0f, 1.0f, 1.0f, 1.0f))};

    // 영향이 없는 정적 부위는 원점으로 사라지지 않도록 바인드 위치를 유지합니다.
    if (Total <= 0.0f) {
        return Position;
    }

    float3 Result = {float3(0.0f, 0.0f, 0.0f)};

    [unroll]
    for (uint Slot = {0}; Slot < 4; ++Slot) {
        Result += mul(float4(Position, 1.0f), SkinBones[BoneIndices[Slot]].mTransform).xyz * Weights[Slot];
    }

    return Result / Total;
}

/** @brief 위치·법선·접선을 같은 본 포즈로 변형합니다. @param Position 입출력 위치입니다. @param Normal 입출력 법선입니다. @param Tangent 입출력 접선과 부호입니다. @param BoneIndices 본 번호입니다. @param Weights 본 영향입니다. */
void SkinVertex(inout float3 Position, inout float3 Normal, inout float4 Tangent, uint4 BoneIndices, float4 Weights) {
    const float Total = {dot(Weights, float4(1.0f, 1.0f, 1.0f, 1.0f))};

    if (Total <= 0.0f) {
        return;
    }

    row_major float4x4 Transform = {(float4x4)0};
    row_major float4x4 NormalTransform = {(float4x4)0};

    [unroll]
    for (uint Slot = {0}; Slot < 4; ++Slot) {
        const float Weight = {Weights[Slot] / Total};
        Transform += SkinBones[BoneIndices[Slot]].mTransform * Weight;
        NormalTransform += SkinBones[BoneIndices[Slot]].mNormalTransform * Weight;
    }

    Position = mul(float4(Position, 1.0f), Transform).xyz;
    Normal = mul(Normal, (float3x3)NormalTransform);
    Tangent = float4(mul(Tangent.xyz, (float3x3)Transform), Tangent.w * (determinant((float3x3)Transform) < 0.0f ? -1.0f : 1.0f));
}
#endif
#endif
