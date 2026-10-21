Texture2D<float4> SceneColor : register(t0);
SamplerState SceneSampler : register(s0);

cbuffer FPostProcessingConstants : register(b0) {
    float4 FXAAParameters;
};

struct FPostProcessingVertex {
    float4 mPosition : SV_POSITION;
    float2 mUV : TEXCOORD0;
};

FPostProcessingVertex MainVS(uint VertexIndex : SV_VertexID) {
    FPostProcessingVertex Output = (FPostProcessingVertex)0;

    Output.mUV = float2((VertexIndex << 1) & 2, VertexIndex & 2);
    Output.mPosition = float4(Output.mUV.x * 2.0f - 1.0f, 1.0f - Output.mUV.y * 2.0f, 0.0f, 1.0f);

    return Output;
}

float CalculateLuminance(float3 Color) {
    return dot(Color, float3(0.299f, 0.587f, 0.114f));
}

float SampleLuminance(float2 UV) {
    return CalculateLuminance(SceneColor.SampleLevel(SceneSampler, UV, 0).rgb);
}

float3 ApplyFXAA(float2 UV, float3 CenterColor) {
    float2 TexelSize = FXAAParameters.xy;
    float Center = CalculateLuminance(CenterColor);
    float North = SampleLuminance(UV + float2(0.0f, -TexelSize.y));
    float South = SampleLuminance(UV + float2(0.0f, TexelSize.y));
    float West = SampleLuminance(UV + float2(-TexelSize.x, 0.0f));
    float East = SampleLuminance(UV + float2(TexelSize.x, 0.0f));
    float Minimum = min(Center, min(min(North, South), min(West, East)));
    float Maximum = max(Center, max(max(North, South), max(West, East)));
    float Contrast = Maximum - Minimum;

    if (Contrast < max(0.0312f, Maximum * 0.125f)) {
        return CenterColor;
    }

    float NorthWest = SampleLuminance(UV + float2(-TexelSize.x, -TexelSize.y));
    float NorthEast = SampleLuminance(UV + float2(TexelSize.x, -TexelSize.y));
    float SouthWest = SampleLuminance(UV + float2(-TexelSize.x, TexelSize.y));
    float SouthEast = SampleLuminance(UV + float2(TexelSize.x, TexelSize.y));
    float Horizontal = abs(North + South - 2.0f * Center) * 2.0f + abs(NorthWest + SouthWest - 2.0f * West) + abs(NorthEast + SouthEast - 2.0f * East);
    float Vertical = abs(West + East - 2.0f * Center) * 2.0f + abs(NorthWest + NorthEast - 2.0f * North) + abs(SouthWest + SouthEast - 2.0f * South);
    bool HorizontalEdge = Horizontal >= Vertical;
    float Negative = HorizontalEdge ? North : West;
    float Positive = HorizontalEdge ? South : East;
    float NegativeGradient = abs(Negative - Center);
    float PositiveGradient = abs(Positive - Center);
    bool NegativePair = NegativeGradient >= PositiveGradient;
    float StepLength = HorizontalEdge ? TexelSize.y : TexelSize.x;

    if (NegativePair) {
        StepLength = -StepLength;
    }

    float PairAverage = 0.5f * (Center + (NegativePair ? Negative : Positive));
    float GradientThreshold = 0.25f * max(NegativeGradient, PositiveGradient);
    float2 EdgeUV = UV + (HorizontalEdge ? float2(0.0f, StepLength * 0.5f) : float2(StepLength * 0.5f, 0.0f));
    float2 TangentStep = HorizontalEdge ? float2(TexelSize.x, 0.0f) : float2(0.0f, TexelSize.y);
    float2 NegativeUV = EdgeUV - TangentStep;
    float2 PositiveUV = EdgeUV + TangentStep;
    float NegativeDelta = SampleLuminance(NegativeUV) - PairAverage;
    float PositiveDelta = SampleLuminance(PositiveUV) - PairAverage;
    bool NegativeFound = abs(NegativeDelta) >= GradientThreshold;
    bool PositiveFound = abs(PositiveDelta) >= GradientThreshold;

    [loop]
    for (uint Iteration = 0; Iteration < 12 && !(NegativeFound && PositiveFound); ++Iteration) {
        float SearchStep = Iteration < 4 ? 1.0f : (Iteration < 8 ? 2.0f : 4.0f);

        if (!NegativeFound) {
            NegativeUV -= TangentStep * SearchStep;
            NegativeDelta = SampleLuminance(NegativeUV) - PairAverage;
            NegativeFound = abs(NegativeDelta) >= GradientThreshold;
        }

        if (!PositiveFound) {
            PositiveUV += TangentStep * SearchStep;
            PositiveDelta = SampleLuminance(PositiveUV) - PairAverage;
            PositiveFound = abs(PositiveDelta) >= GradientThreshold;
        }
    }

    float NegativeDistance = HorizontalEdge ? UV.x - NegativeUV.x : UV.y - NegativeUV.y;
    float PositiveDistance = HorizontalEdge ? PositiveUV.x - UV.x : PositiveUV.y - UV.y;
    bool NegativeCloser = NegativeDistance <= PositiveDistance;
    float NearestDelta = NegativeCloser ? NegativeDelta : PositiveDelta;
    bool NearestFound = NegativeCloser ? NegativeFound : PositiveFound;
    bool CorrectVariation = (NearestDelta < 0.0f) != (Center < PairAverage);
    float EdgeOffset = NearestFound && CorrectVariation ? 0.5f - min(NegativeDistance, PositiveDistance) / (NegativeDistance + PositiveDistance) : 0.0f;
    float NeighborhoodAverage = ((North + South + West + East) * 2.0f + NorthWest + NorthEast + SouthWest + SouthEast) / 12.0f;
    float Subpixel = saturate(abs(NeighborhoodAverage - Center) / Contrast);

    Subpixel = Subpixel * Subpixel * (3.0f - 2.0f * Subpixel);
    Subpixel = Subpixel * Subpixel * 0.75f;

    float Offset = max(EdgeOffset, Subpixel);
    float2 FilteredUV = UV + (HorizontalEdge ? float2(0.0f, Offset * StepLength) : float2(Offset * StepLength, 0.0f));

    return SceneColor.SampleLevel(SceneSampler, FilteredUV, 0).rgb;
}

float4 MainPS(FPostProcessingVertex Input) : SV_TARGET {
    float4 Color = SceneColor.SampleLevel(SceneSampler, Input.mUV, 0);

    if (FXAAParameters.z > 0.5f) {
        Color.rgb = ApplyFXAA(Input.mUV, Color.rgb);
    }

    return Color;
}
