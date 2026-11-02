/**
 * @file Skinning.hlsl
 * @brief 베이스 패스와 그림자 패스가 공유하는 네 본 선형 스키닝입니다.
 */
#ifndef MacawSkinningIncluded
#define MacawSkinningIncluded

#ifdef MacawSkinned
/**
 * @brief CPU에서 본마다 업로드하는 위치 행렬과 법선 역전치 행렬입니다.
 */
struct FSkinBone {
    /**
     * @brief 바인드 위치를 현재 메시 공간 위치로 옮기는 행렬입니다.
     */
    row_major float4x4 mTransform;
    /**
     * @brief 위치 행렬의 역전치로 계산한 법선 변환입니다.
     */
    row_major float4x4 mNormalTransform;
};

/**
 * @brief 재질·조명과 독립된 정점 단계 슬롯의 객체별 본 팔레트입니다.
 */
StructuredBuffer<FSkinBone> SkinBones : register(t10);

/**
 * @brief 정규화한 본 영향으로 바인드 위치를 현재 메시 공간으로 옮깁니다.
 * @param Position 바인드 위치입니다.
 * @param BoneIndices 본 번호입니다.
 * @param Weights 본 영향입니다.
 * @return 현재 포즈의 메시 공간 위치입니다.
 */
float3 SkinPosition(float3 Position, uint4 BoneIndices, float4 Weights) {
    // 깊이 패스처럼 위치만 필요한 경로에서 사용합니다. 변환 결과를 섞는 것과 행렬을 섞은 뒤 변환하는 것은 같으므로 SkinVertex와 위치가 일치합니다.
    const float Total = {dot(Weights, float4(1.0f, 1.0f, 1.0f, 1.0f))};

    // 영향이 없는 정적 부위는 원점으로 사라지지 않도록 바인드 위치를 유지합니다.
    if (Total <= 0.0f) {
        return Position;
    }

    float3 Result = {float3(0.0f, 0.0f, 0.0f)};

    // 본별로 변환한 위치를 가중치로 섞습니다. 쓰지 않는 칸은 가중치가 0이라 결과에 영향을 주지 않습니다.
    [unroll]
    for (uint Slot = {0}; Slot < 4; ++Slot) {
        Result += mul(float4(Position, 1.0f), SkinBones[BoneIndices[Slot]].mTransform).xyz * Weights[Slot];
    }

    return Result / Total;
}

/**
 * @brief 위치·법선·접선을 같은 본 포즈로 변형합니다.
 * @param Position 입출력 위치입니다.
 * @param Normal 입출력 법선입니다.
 * @param Tangent 입출력 접선과 부호입니다.
 * @param BoneIndices 본 번호입니다.
 * @param Weights 본 영향입니다.
 */
void SkinVertex(inout float3 Position, inout float3 Normal, inout float4 Tangent, uint4 BoneIndices, float4 Weights) {
    const float Total = {dot(Weights, float4(1.0f, 1.0f, 1.0f, 1.0f))};

    // 영향이 없는 정적 부위는 SkinPosition과 같이 바인드 형상을 유지합니다.
    if (Total <= 0.0f) {
        return;
    }

    row_major float4x4 Transform = {(float4x4)0};
    row_major float4x4 NormalTransform = {(float4x4)0};

    // 선형 블렌드 스키닝: 네 본의 행렬을 가중치로 섞어 이 정점 전용 행렬을 만듭니다.
    [unroll]
    for (uint Slot = {0}; Slot < 4; ++Slot) {
        const float Weight = {Weights[Slot] / Total};
        Transform += SkinBones[BoneIndices[Slot]].mTransform * Weight;
        NormalTransform += SkinBones[BoneIndices[Slot]].mNormalTransform * Weight;
    }

    // 결과는 현재 포즈의 메시 공간이며, 월드 변환과 법선·접선 정규화는 이어지는 BuildPixelInput이 처리합니다.
    Position = mul(float4(Position, 1.0f), Transform).xyz;
    // 방향 벡터는 이동의 영향을 받지 않으므로 3x3 부분만 곱합니다.
    Normal = mul(Normal, (float3x3)NormalTransform);
    // 접선은 표면을 따라가므로 위치 행렬로 변환하고, 거울 변환이면 종법선이 뒤집히지 않도록 W 부호를 반전합니다.
    Tangent = float4(mul(Tangent.xyz, (float3x3)Transform), Tangent.w * (determinant((float3x3)Transform) < 0.0f ? -1.0f : 1.0f));
}
#endif
#endif
