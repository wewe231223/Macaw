#include "pch.h"
#include "Render/FSceneRenderOutput.h"
#include "Render/IRenderSurface.h"

bool FSceneRenderOutput::IsValid() const {
    return mTarget != nullptr && mTarget->IsValid() && mDepthResource != nullptr && mDepthStencilView != nullptr && mViewport.Width > 0.0f && mViewport.Height > 0.0f;
}
