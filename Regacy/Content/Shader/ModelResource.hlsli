#ifndef MACAW_MODEL_RESOURCE
#define MACAW_MODEL_RESOURCE

struct FMeshDrawRecord {
    uint mObjectIndex;
    uint mMaterialIndex;
    float mLODDither;
    uint mUVTransformIndex;
};

struct FObjectTransform {
    row_major float4x4 mWorld;
};

struct FModelContext {
    row_major float4x4 mWorld;
    uint mMaterialIndex;
    float mLODDither;
    float4 mUVTransform;
};

StructuredBuffer<FMeshDrawRecord> DrawRecords : register(t0);
StructuredBuffer<FObjectTransform> ObjectTransforms : register(t15);
StructuredBuffer<float4> UVTransforms : register(t16);

FModelContext GetModelContext(uint DrawRecordIndex) {
    const FMeshDrawRecord Record = {DrawRecords[DrawRecordIndex]};
    FModelContext Result = {(float4x4)0.0f, 0u, 0.0f, float4(1.0f, 1.0f, 0.0f, 0.0f)};

    Result.mWorld = ObjectTransforms[Record.mObjectIndex].mWorld;

    Result.mMaterialIndex = Record.mMaterialIndex;
    Result.mLODDither = Record.mLODDither;

    if (Record.mUVTransformIndex != 0u) {
        Result.mUVTransform = UVTransforms[Record.mUVTransformIndex];
    }

    return Result;
}

float2 ApplyUVTransform(float2 UV, FModelContext Model) {
    return UV * Model.mUVTransform.xy + Model.mUVTransform.zw;
}

// Both LODs use the same screen-space pattern with complementary tests.
// No time-dependent noise: a stopped camera produces an identical image every frame.
void ApplyLODDither(float2 PixelPosition, float Dither) {
    [branch] if (Dither != 0.0f) {
        const float Noise = frac(52.9829189f * frac(dot(floor(PixelPosition), float2(0.06711056f, 0.00583715f))));
        if (Dither > 0.0f ? Noise < Dither : Noise >= -Dither) { discard; }
    }
}

#endif
