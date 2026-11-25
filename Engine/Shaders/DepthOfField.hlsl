/**
 * @file DepthOfField.hlsl
 * @brief 픽셀별 착란원 반경으로 전경과 배경을 절반 해상도에서 흐리게 한 뒤 원본 장면에 합성합니다.
 * @details 블러는 scatter-as-gather입니다. 각 표본은 자신의 착란원 반경만큼의 조리개 모양으로 빛을 퍼뜨린다고 보고,
 *          출력 텍셀은 자신을 덮는 표본만 모읍니다. 따라서 초점에서 멀수록 흐림 반경이 커집니다.
 */

#include "PostProcess.hlsl"

Texture2D<float4> ForegroundLayer : register(t2);
Texture2D<float4> BackgroundLayer : register(t3);

/** @brief 절반 해상도 텍셀의 전경(R)·배경(G) 착란원 반경이며 절반 해상도 텍셀 단위입니다. */
Texture2D<float4> CircleOfConfusion : register(t4);

struct FDepthOfFieldSeparation {
    float4 mForeground : SV_TARGET0;
    float4 mBackground : SV_TARGET1;
    float4 mCircleOfConfusion : SV_TARGET2;
};

static const float MinLayerCoverage = {0.0001f};
static const int2 NeighborOffsets[8] = {
    int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1),
    int2(-1, -1), int2(1, -1), int2(-1, 1), int2(1, 1)
};

// 오각형 각 변의 바깥 법선입니다. 꼭짓점이 화면 위(-Y)를 향하고 X 대칭이므로 |X|에 대한 세 변만 사용합니다.
static const float2 PentagonEdgeNormals[3] = {float2(0.5877853f, -0.8090170f), float2(0.9510565f, 0.3090170f), float2(0.0f, 1.0f)};
static const float PentagonApothem = {0.8090170f};
// 꼭짓점이 위아래를 향하는 육각형의 변 법선과 아포템입니다.
static const float2 HexagonEdgeNormals[2] = {float2(1.0f, 0.0f), float2(0.5f, 0.8660254f)};
static const float HexagonApothem = {0.8660254f};

/**
 * @brief 조리개 중심에서 Position까지의 거리를 조리개 모양으로 잽니다. 외접원 반경 r인 조리개의 경계에서 r이 됩니다.
 * @details 다각형은 각 변까지의 거리 중 최대값(게이지 함수)을 사용합니다. 길이에 비례하므로 반경과 바로 비교할 수 있습니다.
 */
float ApertureDistance(float2 Position, float Sides) {
    const float2 Mirrored = {abs(Position.x), Position.y};

    if (Sides == 5.0f) {
        float Distance = {0.0f};

        [unroll]
        for (int Edge = 0; Edge < 3; ++Edge) {
            Distance = max(Distance, dot(Mirrored, PentagonEdgeNormals[Edge]));
        }

        return Distance / PentagonApothem;
    }

    if (Sides == 6.0f) {
        const float2 Folded = {abs(Position)};

        return max(dot(Folded, HexagonEdgeNormals[0]), dot(Folded, HexagonEdgeNormals[1])) / HexagonApothem;
    }

    return length(Position);
}

/** @brief 외접원 반경 1인 조리개의 넓이입니다. 표본이 퍼뜨리는 빛을 이 넓이로 나눠 밝기를 보존합니다. */
float ApertureArea(float Sides) {
    return Sides == 5.0f ? 2.3776413f : Sides == 6.0f ? 2.5980762f : 3.1415927f;
}

// LDR 장면 색에서 잘린 광원 에너지를 근사로 복원하는 입력 상한입니다. 0.92이면 최대 12.5배까지 펼칩니다.
static const float BokehHighlightInputLimit = {0.92f};

// 블러 전에 LDR 색을 역 Reinhard로 HDR 근사값으로 펼쳐 작은 광원이 밝은 보케 디스크로 남게 합니다.
float3 ExpandBokehHighlights(float3 Color) {
    const float Peak = {min(max(max(Color.r, Color.g), Color.b), BokehHighlightInputLimit)};

    return Color / (1.0f - Peak);
}

// ExpandBokehHighlights의 정확한 역함수입니다. 흐려지지 않은 색은 원래 값으로 돌아옵니다.
float3 CompressBokehHighlights(float3 Color) {
    return Color / (1.0f + max(max(Color.r, Color.g), Color.b));
}

/** @brief 전경 착란원 반경의 상한이자 전경 블러의 탐색 반경(px)입니다. */
float ForegroundSearchRadius() {
    return DepthOfFieldParameters.z;
}

/** @brief 배경 블러의 탐색 반경(px)입니다. 배경 착란원은 무한대 반경 K를 넘지 않습니다. */
float BackgroundSearchRadius() {
    return DepthOfFieldParameters.w;
}

/** @brief 장치 깊이의 착란원 반경(px)입니다. 하늘(깊이 1)은 무한대 반경이며 상한으로 자릅니다. */
float GetCircleOfConfusion(float Depth) {
    const float Radius = {Depth < 1.0f ? ComputeCircleOfConfusionRadius(ReconstructViewDepth(Depth), DepthOfFieldParameters.x, DepthOfFieldParameters.y) : DepthOfFieldParameters.y};

    return min(Radius, ForegroundSearchRadius());
}

/** @brief 장치 깊이와 착란원 반경으로 전경·배경 레이어 가중치를 구하고 Near·Far 토글을 적용합니다. */
float2 GetLayerWeights(float Depth, float Radius) {
    const float2 Weights = {Depth < 1.0f ? ComputeDepthOfFieldWeights(ReconstructViewDepth(Depth), Radius, DepthOfFieldParameters.x) : float2(0.0f, saturate(Radius - DepthOfFieldSharpRadius))};

    return Weights * (1.0f - Settings.zw);
}

FDepthOfFieldSeparation SeparateDepthOfField(FVertexOutput Input) {
    const int2 FirstPixel = {int2(Input.mPosition.xy) * 2};
    const int2 LastPixel = {int2(ScreenSize.xy) - 1};
    FDepthOfFieldSeparation Output = {(FDepthOfFieldSeparation)0};
    float2 RadiusSum = {(float2)0};
    float2 WeightSum = {(float2)0};

    [unroll]
    for (int Y = 0; Y < 2; ++Y) {
        [unroll]
        for (int X = 0; X < 2; ++X) {
            const int3 Pixel = {int3(min(FirstPixel + int2(X, Y), LastPixel), 0)};
            const float Depth = {SceneDepth.Load(Pixel)};
            const float Radius = {GetCircleOfConfusion(Depth)};
            const float2 Weights = {GetLayerWeights(Depth, Radius)};
            const float4 Color = {float4(ExpandBokehHighlights(SceneColor.Load(Pixel).rgb), 1.0f)};

            // 색상과 커버리지를 함께 필터링해 레이어 경계에서 색이 섞이거나 어두워지는 현상을 줄입니다.
            Output.mForeground += Color * Weights.x;
            Output.mBackground += Color * Weights.y;
            RadiusSum += Radius * Weights;
            WeightSum += Weights;
        }
    }

    Output.mForeground *= 0.25f;
    Output.mBackground *= 0.25f;
    // 착란원 반경은 레이어에 속한 픽셀끼리 평균하고 절반 해상도 텍셀 단위로 바꿉니다.
    Output.mCircleOfConfusion = float4(0.5f * RadiusSum / max(WeightSum, MinLayerCoverage), 0.0f, 0.0f);

    return Output;
}

// 한 번에 한 레이어만 흐리게 하므로 그 레이어 하나만 출력합니다. 쓰지 않는 출력 슬롯을 비워 두면 D3D11 경고가 납니다.
float4 BlurDepthOfField(FVertexOutput Input) : SV_TARGET {
    const bool BlurForeground = {Settings.x == 0.0f};
    const float Sides = {Settings.y};
    const float2 HalfSize = {ceil(ScreenSize.xy * 0.5f)};
    const float2 TexelUV = {1.0f / HalfSize};
    // 이 레이어에서 가장 큰 착란원 반경(절반 해상도 텍셀)까지 찾습니다.
    const float SearchRadius = {max(0.5f * (BlurForeground ? ForegroundSearchRadius() : BackgroundSearchRadius()), 0.5f)};
    // 큰 반경에서는 표본 간격을 넓혀 한 축의 표본 수를 약 21개로 제한합니다.
    const float SampleSpacing = {max(1.0f, SearchRadius / 10.0f)};
    const int Extent = {(int)ceil(SearchRadius / SampleSpacing)};
    // 표본 하나가 대표하는 넓이를 조리개 넓이로 나눈 값입니다. 반경 r 디스크의 넓이는 ApertureArea · r²입니다.
    const float SampleShare = {SampleSpacing * SampleSpacing / ApertureArea(Sides)};
    float4 Sum = {(float4)0};
    float TotalWeight = {0.0f};

    [loop]
    for (int Y = -Extent; Y <= Extent; ++Y) {
        [loop]
        for (int X = -Extent; X <= Extent; ++X) {
            const float2 Offset = {float2(X, Y) * SampleSpacing};
            // 표본 q가 출력 텍셀 p를 덮으려면 p − q가 q의 조리개 안에 있어야 합니다.
            const float Distance = {ApertureDistance(-Offset, Sides)};

            if (Distance > SearchRadius + SampleSpacing) {
                continue;
            }

            const float2 UV = {Input.mUV + Offset * TexelUV};
            const float2 Radii = {CircleOfConfusion.SampleLevel(LinearClamp, UV, 0.0f).rg};
            // 표본 간격보다 작은 디스크는 자기 텍셀만 덮는 것으로 봅니다.
            const float Radius = {max(BlurForeground ? Radii.x : Radii.y, 0.5f * SampleSpacing)};
            // 경계를 표본 간격 폭으로 부드럽게 해 반경이 연속적으로 변할 때 계단이 생기지 않게 합니다.
            const float Cover = {saturate((Radius - Distance) / SampleSpacing + 0.5f)};

            if (Cover <= 0.0f) {
                continue;
            }

            // 표본의 빛을 디스크 넓이에 고르게 나누므로 큰 디스크일수록 텍셀 하나에 주는 몫이 작습니다.
            const float Weight = {Cover * SampleShare / (Radius * Radius)};
            const float4 Sample = {BlurForeground ? ForegroundLayer.SampleLevel(LinearClamp, UV, 0.0f) : BackgroundLayer.SampleLevel(LinearClamp, UV, 0.0f)};

            Sum += Sample * Weight;
            TotalWeight += Weight;
        }
    }

    if (BlurForeground) {
        // 전경은 덮은 디스크들의 불투명도를 그대로 누적합니다. 흐린 전경이 원래 실루엣 밖으로 반투명하게 번집니다.
        return Sum / max(Sum.a, 1.0f);
    }

    // 배경은 뒤를 모두 채우므로 가중 평균합니다. 합성에서 RGB를 알파로 나눠 색을 복원합니다.
    return Sum / max(TotalWeight, 0.000001f);
}

float3 CompositeBackground(float3 Color, float BackgroundWeight, float2 LayerUV) {
    // 전체 해상도 깊이로 배경을 합성해 초점 물체의 경계를 보존합니다.
    [branch]
    if (BackgroundWeight > 0.0f) {
        const float4 Background = {BackgroundLayer.SampleLevel(LinearClamp, LayerUV, 0.0f)};

        if (Background.a > MinLayerCoverage) {
            Color = lerp(Color, CompressBokehHighlights(Background.rgb / Background.a), BackgroundWeight);
        }
    }

    return Color;
}

float3 ReconstructBehindForeground(int2 Pixel, float Depth, float Radius, float2 HalfSize, float3 Fallback) {
    const int2 LastPixel = {int2(ScreenSize.xy) - 1};
    float4 Behind = {(float4)0};

    [loop]
    for (int Ring = 0; Ring < 2; ++Ring) {
        const float Distance = {Ring == 0 ? max(1.0f, Radius * 0.4f) : Radius + 2.0f};
        const float Weight = {Ring == 0 ? 2.0f : 1.0f};
        const int AxialOffset = {(int)round(Distance)};
        const int DiagonalOffset = {(int)round(Distance * 0.70710678f)};

        [loop]
        for (int Index = 0; Index < 8; ++Index) {
            const int2 Offset = {NeighborOffsets[Index] * (Index < 4 ? AxialOffset : DiagonalOffset)};
            const int2 Neighbor = {clamp(Pixel + Offset, int2(0, 0), LastPixel)};
            const int3 SamplePixel = {int3(Neighbor, 0)};
            const float SampleDepth = {SceneDepth.Load(SamplePixel)};
            const float2 Weights = {GetLayerWeights(SampleDepth, GetCircleOfConfusion(SampleDepth))};

            if (SampleDepth > Depth && Weights.x <= MinLayerCoverage) {
                const float2 LayerUV = {(float2(Neighbor) + 0.5f) * 0.5f / HalfSize};
                const float3 Color = {CompositeBackground(SceneColor.Load(SamplePixel).rgb, Weights.y, LayerUV)};
                Behind += float4(Color, 1.0f) * Weight;
            }
        }
    }

    return Behind.a > 0.0f ? Behind.rgb / Behind.a : Fallback;
}

float4 CompositeDepthOfField(FVertexOutput Input) : SV_TARGET {
    const int2 Pixel = {int2(Input.mPosition.xy)};
    const float4 Original = {SceneColor.Load(int3(Pixel, 0))};
    const float Depth = {SceneDepth.Load(int3(Pixel, 0))};
    const float Radius = {GetCircleOfConfusion(Depth)};
    const float2 Weights = {GetLayerWeights(Depth, Radius)};
    const float2 HalfSize = {ceil(ScreenSize.xy * 0.5f)};
    const float2 LayerUV = {Input.mPosition.xy * 0.5f / HalfSize};
    float3 Color = {CompositeBackground(Original.rgb, Weights.y, LayerUV)};

    const float4 Foreground = {ForegroundLayer.SampleLevel(LinearClamp, LayerUV, 0.0f)};
    const float Coverage = {saturate(Foreground.a)};
    const float Uncovered = {1.0f - Coverage};
    const float MissingCoverage = {Weights.x - Coverage};
    // 레이어는 HDR 근사값을 커버리지로 곱해 저장하므로 색을 되돌린 뒤 다시 커버리지를 곱합니다.
    const float3 NearColor = {Foreground.a > MinLayerCoverage ? CompressBokehHighlights(Foreground.rgb / Foreground.a) : (float3)0};

    [branch]
    if (Foreground.a > 0.0f && MissingCoverage > MinLayerCoverage) {
        // 흐린 전경이 퍼지며 얇아진 영역은 뒤쪽 표본으로 채워 원본의 선명한 전경 윤곽이 비치지 않게 합니다.
        const float3 Behind = {ReconstructBehindForeground(Pixel, Depth, Radius, HalfSize, NearColor)};
        Color = lerp(Color, Behind, saturate(MissingCoverage / max(Uncovered, MinLayerCoverage)));
    }

    return float4(NearColor * Coverage + Color * Uncovered, Original.a);
}
