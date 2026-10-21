#pragma once

#include "Engine/FSceneView.h"

class FSceneInterface;
class FRenderTarget;
class FPrimitiveDrawInterface;

struct FSceneViewFamily {
    const FSceneInterface* mScene{};
    FRenderTarget* mTarget{};

    FSceneView mView{};

    const FPrimitiveDrawInterface* mEditorElements{};
};
