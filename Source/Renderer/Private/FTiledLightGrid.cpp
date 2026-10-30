/**
 * @file FTiledLightGrid.cpp
 * @brief 16×16픽셀 타일마다 영향을 주는 광원의 인덱스 목록을 구성합니다.
 * @ingroup Renderer
 */

#include "FTiledLightGrid.h"
#include "Core/Stats.h"
#include "Core/Math.h"

#include <algorithm>
#include <cmath>
#include <limits>

void FTiledLightGrid::Build(std::span<const FLightSceneProxy> Lights, const FSceneView& View, std::span<const std::array<Uint32, 2>> ShadowRanges) {
    static FStat LightGridTime{"render", "Tiled light grid CPU", EStatUnit::Milliseconds};
    static FStat SubmittedLights{"render", "Input lights (sum of views)"};
    const FScopedStatTimer StatTimer{LightGridTime};
    SubmittedLights.Add(static_cast<double>(Lights.size()));
    mLights.clear();
    mRanges.clear();
    mIndices.clear();
    mColumns = (static_cast<Uint32>(std::max(View.mSize.x, 1.0f)) + 15) / 16;
    mRows = (static_cast<Uint32>(std::max(View.mSize.y, 1.0f)) + 15) / 16;

    TArray<TArray<Uint32>> Tiles{};
    Tiles.resize(static_cast<std::size_t>(mColumns) * mRows);

    for (std::size_t SourceIndex{}; SourceIndex < Lights.size(); ++SourceIndex) {
        const FLightSceneProxy& Light{Lights[SourceIndex]};

        if (!Light.mVisible || Light.mIntensity <= 0.0f) {
            continue;
        }

        const Uint32 LightIndex{static_cast<Uint32>(mLights.size())};

        mLights.push_back({{Light.mPosition, Light.mAttenuationRadius}, {Light.mDirection, static_cast<float>(Light.mType)}, {Light.mColor.x, Light.mColor.y, Light.mColor.z, Light.mIntensity}, {Light.mLightFalloffExponent, std::cos(Math::ToRadians(Light.mInnerConeAngle)), std::cos(Math::ToRadians(Light.mOuterConeAngle)), 0.0f}});

        if (SourceIndex < ShadowRanges.size()) {
            mLights.back().mShadowInfo = {static_cast<float>(ShadowRanges[SourceIndex][0]), static_cast<float>(ShadowRanges[SourceIndex][1]), 0.0f, 0.0f};
        }

        Uint32 MinimumX{};
        Uint32 MinimumY{};
        Uint32 MaximumX{mColumns - 1};
        Uint32 MaximumY{mRows - 1};

        if (Light.mType == ELightType::Point || Light.mType == ELightType::Spot) {
            float Left{std::numeric_limits<float>::max()};
            float Top{Left};
            float Right{-Left};
            float Bottom{-Left};
            Uint32 Behind{};
            Uint32 BeforeNear{};
            Uint32 BeyondFar{};

            for (Uint32 Corner{}; Corner < 8; ++Corner) {
                const FVector Offset{(Corner & 1) ? Light.mAttenuationRadius : -Light.mAttenuationRadius, (Corner & 2) ? Light.mAttenuationRadius : -Light.mAttenuationRadius, (Corner & 4) ? Light.mAttenuationRadius : -Light.mAttenuationRadius};
                const FVector4 Clip{FVector4{Light.mPosition + Offset, 1.0f} * View.mViewProjection};
                Behind += Clip.w <= 0.0001f;
                BeforeNear += Clip.z < 0.0f;
                BeyondFar += Clip.z > Clip.w;

                if (Clip.w > 0.0001f) {
                    const float X{(Clip.x / Clip.w * 0.5f + 0.5f) * View.mSize.x};
                    const float Y{(0.5f - Clip.y / Clip.w * 0.5f) * View.mSize.y};
                    Left = std::min(Left, X);
                    Right = std::max(Right, X);
                    Top = std::min(Top, Y);
                    Bottom = std::max(Bottom, Y);
                }
            }

            if (Behind == 8 || BeforeNear == 8 || BeyondFar == 8) {
                continue;
            }

            if (Behind == 0) {
                if (Right < 0.0f || Bottom < 0.0f || Left >= View.mSize.x || Top >= View.mSize.y) {
                    continue;
                }

                MinimumX = static_cast<Uint32>(std::max(Left, 0.0f) / 16.0f);
                MinimumY = static_cast<Uint32>(std::max(Top, 0.0f) / 16.0f);
                MaximumX = std::min(static_cast<Uint32>(std::max(Right, 0.0f) / 16.0f), mColumns - 1);
                MaximumY = std::min(static_cast<Uint32>(std::max(Bottom, 0.0f) / 16.0f), mRows - 1);
            }
        }

        for (Uint32 Y{MinimumY}; Y <= MaximumY; ++Y) {
            for (Uint32 X{MinimumX}; X <= MaximumX; ++X) {
                Tiles[static_cast<std::size_t>(Y) * mColumns + X].push_back(LightIndex);
            }
        }
    }

    mRanges.reserve(Tiles.size());

    for (const auto& Tile : Tiles) {
        mRanges.push_back({static_cast<Uint32>(mIndices.size()), static_cast<Uint32>(Tile.size())});
        mIndices.insert(mIndices.end(), Tile.begin(), Tile.end());
    }
}

const TArray<FGpuLight>& FTiledLightGrid::GetLights() const {
    return mLights;
}

const TArray<std::array<Uint32, 2>>& FTiledLightGrid::GetRanges() const {
    return mRanges;
}

const TArray<Uint32>& FTiledLightGrid::GetIndices() const {
    return mIndices;
}

Uint32 FTiledLightGrid::GetColumns() const {
    return mColumns;
}

Uint32 FTiledLightGrid::GetRows() const {
    return mRows;
}
