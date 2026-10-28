cbuffer ObjectConstants : register(b0) {
    row_major float4x4 World;
    row_major float4x4 WorldInverse;
    float4 UVTransform;
};

#include "ViewConstants.hlsli"

cbuffer MaterialConstants : register(b2) {
    float4 BaseColor;
};

Texture2D BaseColorTexture : register(t0);
SamplerState MaterialSampler : register(s0);

struct FInput {
    float3 mPosition : POSITION;
    float3 mNormal : NORMAL;
    float2 mUV : TEXCOORD0;
    float4 mColor : COLOR;
};

struct FOutput {
    float4 mPosition : SV_POSITION;
    float3 mNormal : NORMAL;
    float2 mUV : TEXCOORD0;
    float4 mColor : COLOR;
};
