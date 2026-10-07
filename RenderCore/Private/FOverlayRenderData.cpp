#include "pch.h"
#include "RenderCore/FOverlayRenderData.h"

void FOverlayRenderData::Clear() {
    mSelectionProbes.clear();
    mGizmoProbes.clear();
    mTextProbes.clear();
    mGuides.Clear();
    mGridFade = {};
}

bool FOverlayRenderData::IsPassEnabled(EOverlayPass Pass) const {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    return Index < mPasses.size() && mPasses.test(Index);
}

void FOverlayRenderData::SetPassEnabled(EOverlayPass Pass, bool Enabled) {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    if (Index < mPasses.size()) {
        mPasses.set(Index, Enabled);
    }
}
