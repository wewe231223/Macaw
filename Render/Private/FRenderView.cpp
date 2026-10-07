#include "pch.h"
#include "Render/FRenderView.h"

bool FRenderView::IsPassEnabled(ERenderPass Pass) const {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    if (Index >= mPasses.size() || !mPasses.test(Index)) {
        return false;
    }

    return true;
}

void FRenderView::SetPassEnabled(ERenderPass Pass, bool Enabled) {
    const std::size_t Index{static_cast<std::size_t>(Pass)};

    if (Index < mPasses.size()) {
        mPasses.set(Index, Enabled);
    }
}
