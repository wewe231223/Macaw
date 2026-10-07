Texture2D SelectionMask : register(t3);

#include "ModelResource.hlsli"
#include "FrameResource.hlsli"

struct FMaskVertex {
    float4 mPosition : SV_POSITION;
    nointerpolation float mLODDither : TEXCOORD7;
};

FMaskVertex MainMaskVS(float3 Position : POSITION, uint DrawRecordIndex : MODEL_INDEX) {
    const FModelContext ModelContext = {GetModelContext(DrawRecordIndex)};
    const float4 WorldPosition = {mul(float4(Position, 1.0f), ModelContext.mWorld)};
    FMaskVertex Output = {(FMaskVertex)0};

    Output.mPosition = mul(WorldPosition, ViewProjection);
    Output.mLODDither = ModelContext.mLODDither;

    return Output;
}

float4 MainVS(uint VertexIndex : SV_VertexID) : SV_POSITION {
    const float2 Position = {float2((VertexIndex << 1) & 2, VertexIndex & 2)};

    return float4(Position * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
}

float MainMaskPS(FMaskVertex Input) : SV_TARGET {
    ApplyLODDither(Input.mPosition.xy, Input.mLODDither);

    return 1.0f;
}

float4 MainOutlinePS(float4 Position : SV_POSITION) : SV_TARGET {
    uint Width = {0};
    uint Height = {0};

    SelectionMask.GetDimensions(Width, Height);

    const int2 Pixel = {int2(Position.xy)};
    const float Center = {SelectionMask.Load(int3(Pixel, 0)).r};
    float Coverage = {0.0f};

    [unroll] for (int Y = {-2}; Y <= 2; ++Y) {
        [unroll] for (int X = {-2}; X <= 2; ++X) {
            const int2 SamplePixel = {clamp(Pixel + int2(X, Y), int2(0, 0), int2(Width, Height) - 1)};

            Coverage = max(Coverage, SelectionMask.Load(int3(SamplePixel, 0)).r);
        }
    }

    clip(Coverage - Center - 0.5f);

    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}
