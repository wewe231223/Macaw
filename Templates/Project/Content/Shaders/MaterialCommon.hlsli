cbuffer ObjectConstants : register(b0) {
    row_major float4x4 World;
    row_major float4x4 WorldInverse;
    float4 UVTransform;
};

#include "ViewConstants.hlsli"

cbuffer MaterialConstants : register(b2) {
    float4 BaseColor;
    float4 SurfaceParameters;
};

Texture2D BaseColorTexture : register(t0);
#ifdef HAS_NORMAL_MAP
Texture2D NormalMapTexture : register(t1);
#endif
SamplerState MaterialSampler : register(s0);

struct FInput {
    float3 mPosition : POSITION;
    float3 mNormal : NORMAL;
    float4 mTangent : TANGENT;
    float2 mUV : TEXCOORD0;
    float4 mColor : COLOR;
};

struct FOutput {
    float4 mPosition : SV_POSITION;
    float3 mNormal : NORMAL;
    float4 mTangent : TANGENT;
    float2 mUV : TEXCOORD0;
    float3 mWorldPosition : TEXCOORD1;
    float3 mDiffuse : TEXCOORD2;
    float3 mSpecular : TEXCOORD3;
    float4 mColor : COLOR;
};
