#include "ViewConstants.hlsli"

cbuffer LineConstants : register(b0) {
    float4 Viewport;
};

struct FInput {
    float4 mStartAndWidth : START;
    float4 mEnd : END;
    float4 mColor : COLOR;
};

struct FOutput {
    float4 mPosition : SV_POSITION;
    float4 mColor : COLOR;
    noperspective float mEdgeDistance : TEXCOORD0;
    nointerpolation float mHalfWidth : TEXCOORD1;
};

FOutput VertexMain(FInput Input, uint Vertex : SV_VertexID) {
    FOutput Output = (FOutput)0;
    float4 Start = mul(float4(Input.mStartAndWidth.xyz, 1.0f), ViewProjection);
    float4 End = mul(float4(Input.mEnd.xyz, 1.0f), ViewProjection);
    float StartDistance = Start.z - 0.00001f;
    float EndDistance = End.z - 0.00001f;
    if (StartDistance < 0.0f && EndDistance < 0.0f) {
        Output.mPosition = float4(0.0f, 0.0f, -1.0f, 1.0f);
        return Output;
    }
    if (StartDistance < 0.0f) {
        Start = lerp(Start, End, saturate(-StartDistance / (EndDistance - StartDistance)));
    } else if (EndDistance < 0.0f) {
        End = lerp(End, Start, saturate(-EndDistance / (StartDistance - EndDistance)));
    }
    float2 Direction = (End.xy / End.w - Start.xy / Start.w) * float2(Viewport.x, -Viewport.y);
    float Length = length(Direction);
    Direction = Length > 0.0001f ? Direction / Length : float2(1.0f, 0.0f);
    const float2 Corners[6] = {float2(0.0f, -1.0f), float2(0.0f, 1.0f), float2(1.0f, -1.0f), float2(1.0f, -1.0f), float2(0.0f, 1.0f), float2(1.0f, 1.0f)};
    float2 Corner = Corners[Vertex];
    float HalfWidth = max(Input.mStartAndWidth.w * 0.5f, 0.5f);
    float4 Position = Corner.x < 0.5f ? Start : End;
    Position.xy += float2(-Direction.y, Direction.x) * Corner.y * (HalfWidth + 0.5f) * float2(2.0f * Viewport.z, -2.0f * Viewport.w) * Position.w;
    Output.mPosition = Position;
    Output.mColor = Input.mColor;
    Output.mEdgeDistance = Corner.y * (HalfWidth + 0.5f);
    Output.mHalfWidth = HalfWidth;
    return Output;
}

float4 PixelMain(FOutput Input) : SV_TARGET {
    float Coverage = saturate(Input.mHalfWidth + 0.5f - abs(Input.mEdgeDistance));
    clip(Input.mColor.a * Coverage - 0.001f);
    return float4(Input.mColor.rgb, Input.mColor.a * Coverage);
}
