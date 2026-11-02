#pragma pack_matrix(row_major)

#include "LightData.hlsli"

cbuffer CullingConstants : register(b0) {
    float4x4 InverseViewProjection;
    uint4 TileDimensions;
    float4 ScreenDimensions;
};

StructuredBuffer<FLight> Lights : register(t0);
RWStructuredBuffer<uint2> TileRanges : register(u0);
RWStructuredBuffer<uint> TileLightIndices : register(u1);
RWStructuredBuffer<uint> Allocator : register(u2);

groupshared float4 FrustumPlanes[6];
groupshared uint FrustumValid;
groupshared uint VisibleCount;
groupshared uint ListStart;

float4 MakePlane(float3 A, float3 B, float3 C, float3 Inside) {
    float3 Normal = {cross(B - A, C - A)};
    float LengthSquared = {dot(Normal, Normal)};

    if (!isfinite(LengthSquared) || LengthSquared <= 0.000000000001f) {
        FrustumValid = 0;
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    Normal *= rsqrt(LengthSquared);
    float4 Plane = {float4(Normal, -dot(Normal, A))};
    return dot(Plane, float4(Inside, 1.0f)) < 0.0f ? -Plane : Plane;
}

void BuildFrustum(uint2 Tile) {
    uint2 Minimum = {Tile * 16};
    uint2 Maximum = {min(Minimum + 16, (uint2)ScreenDimensions.xy)};
    float2 Lower = {float2(Minimum) / ScreenDimensions.xy * 2.0f - 1.0f};
    float2 Upper = {float2(Maximum) / ScreenDimensions.xy * 2.0f - 1.0f};
    float2 Corners[4] = {float2(Lower.x, -Lower.y), float2(Upper.x, -Lower.y), float2(Lower.x, -Upper.y), float2(Upper.x, -Upper.y)};
    float3 NearCorners[4] = {float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f)};
    float3 FarCorners[4] = {float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 0.0f)};
    float3 Center = {float3(0.0f, 0.0f, 0.0f)};
    FrustumValid = 1;

    for (uint Corner = {0}; Corner < 4; ++Corner) {
        float4 Near = {mul(float4(Corners[Corner], 0.0f, 1.0f), InverseViewProjection)};
        float4 Far = {mul(float4(Corners[Corner], 1.0f, 1.0f), InverseViewProjection)};

        if (!all(isfinite(Near)) || !all(isfinite(Far)) || abs(Near.w) < 0.00000001f || abs(Far.w) < 0.00000001f) {
            FrustumValid = 0;
            return;
        }

        NearCorners[Corner] = Near.xyz / Near.w;
        FarCorners[Corner] = Far.xyz / Far.w;
        Center += NearCorners[Corner] + FarCorners[Corner];
    }

    Center *= 0.125f;
    FrustumPlanes[0] = MakePlane(NearCorners[0], NearCorners[2], FarCorners[2], Center);
    FrustumPlanes[1] = MakePlane(NearCorners[3], NearCorners[1], FarCorners[1], Center);
    FrustumPlanes[2] = MakePlane(NearCorners[1], NearCorners[0], FarCorners[0], Center);
    FrustumPlanes[3] = MakePlane(NearCorners[2], NearCorners[3], FarCorners[3], Center);
    FrustumPlanes[4] = MakePlane(NearCorners[0], NearCorners[1], NearCorners[3], Center);
    FrustumPlanes[5] = MakePlane(FarCorners[1], FarCorners[0], FarCorners[2], Center);
}

bool IntersectsSpot(FLight Light) {
    float Height = {Light.mPositionAndRadius.w};
    float DirectionLengthSquared = {dot(Light.mDirectionAndType.xyz, Light.mDirectionAndType.xyz)};

    if (Height <= 0.0f) {
        return false;
    }

    if (DirectionLengthSquared <= 0.00000001f) {
        return true;
    }

    float3 Direction = {Light.mDirectionAndType.xyz * rsqrt(DirectionLengthSquared)};
    float Cosine = {max(Light.mFalloffAndCones.z, 0.0001f)};
    float Radius = {Height * sqrt(saturate(1.0f - Cosine * Cosine)) / Cosine};
    float3 BaseCenter = {Light.mPositionAndRadius.xyz + Direction * Height};
    float CenterDistance = {(Height * Height + Radius * Radius) / (2.0f * Height)};
    float3 SphereCenter = {Radius <= Height ? Light.mPositionAndRadius.xyz + Direction * CenterDistance : BaseCenter};
    float SphereRadius = {Radius <= Height ? CenterDistance : Radius};

    for (uint PlaneIndex = {0}; PlaneIndex < 6; ++PlaneIndex) {
        float4 Plane = {FrustumPlanes[PlaneIndex]};

        if (dot(Plane.xyz, SphereCenter) + Plane.w < -SphereRadius) {
            return false;
        }
    }

    for (uint ConePlane = {0}; ConePlane < 6; ++ConePlane) {
        float4 Plane = {FrustumPlanes[ConePlane]};
        float Along = {dot(Plane.xyz, Direction)};
        float Apex = {dot(Plane.xyz, Light.mPositionAndRadius.xyz) + Plane.w};
        float Base = {dot(Plane.xyz, BaseCenter) + Plane.w + Radius * sqrt(saturate(1.0f - Along * Along))};

        if (max(Apex, Base) < 0.0f) {
            return false;
        }
    }

    return true;
}

bool IntersectsLight(FLight Light) {
    uint Type = {(uint)Light.mDirectionAndType.w};

    if (Type < 2 || FrustumValid == 0) {
        return true;
    }

    if (Type == 3) {
        return IntersectsSpot(Light);
    }

    for (uint PlaneIndex = {0}; PlaneIndex < 6; ++PlaneIndex) {
        float4 Plane = {FrustumPlanes[PlaneIndex]};

        if (dot(Plane.xyz, Light.mPositionAndRadius.xyz) + Plane.w < -Light.mPositionAndRadius.w) {
            return false;
        }
    }

    return true;
}

[numthreads(64, 1, 1)]
void ComputeMain(uint3 Group : SV_GroupID, uint Thread : SV_GroupIndex) {
    if (Thread == 0) {
        BuildFrustum(Group.xy);
        VisibleCount = 0;
        ListStart = 0;
    }

    GroupMemoryBarrierWithGroupSync();
    uint Count = {0};

    for (uint LightIndex = {Thread}; LightIndex < TileDimensions.z; LightIndex += 64) {
        Count += IntersectsLight(Lights[LightIndex]) ? 1 : 0;
    }

    InterlockedAdd(VisibleCount, Count);
    GroupMemoryBarrierWithGroupSync();

    if (Thread == 0) {
        InterlockedAdd(Allocator[0], VisibleCount, ListStart);
        TileRanges[Group.y * TileDimensions.x + Group.x] = uint2(ListStart, VisibleCount);
        VisibleCount = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint Index = {Thread}; Index < TileDimensions.z; Index += 64) {
        if (IntersectsLight(Lights[Index])) {
            uint WriteIndex = {0};
            InterlockedAdd(VisibleCount, 1, WriteIndex);
            TileLightIndices[ListStart + WriteIndex] = Index;
        }
    }
}
