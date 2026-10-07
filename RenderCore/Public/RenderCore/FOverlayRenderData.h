#pragma once

#include <bitset>
#include "RenderCore/FRenderProbe.h"
#include "RenderCore/FLineRenderData.h"

enum class EOverlayPass : Uint8 {
    SelectionOutline,
    Guides,
    Gizmo,
    Text,
    OrientationAxis,
    Count
};

struct FOverlayTextProbe {
    FVector3 mWorldAnchor{};
    FVector3 mWorldBoundsExtent{};
    FVector2 mScreenOffset{0.0f, -8.0f};
    FAssetHandle mFontHandle{};
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    FString mText{};
    float mPixelHeight{24.0f};
    float mLetterSpacing{};
    float mLineSpacing{};
};

struct FOverlayRenderData {
    bool IsPassEnabled(EOverlayPass Pass) const;
    void SetPassEnabled(EOverlayPass Pass, bool Enabled);

    std::bitset<static_cast<std::size_t>(EOverlayPass::Count)> mPasses{(1ull << static_cast<std::size_t>(EOverlayPass::Count)) - 1};
    float mOrientationAxisSize{};
    TArray<FActorProbe> mSelectionProbes{};
    TArray<FActorProbe> mGizmoProbes{};
    TArray<FOverlayTextProbe> mTextProbes{};
    FLineRenderData mGuides{};
    FVector4 mGridFade{};
};
