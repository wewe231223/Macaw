#include "Engine/FSceneViewport.h"
#include "Core/Math/FVector2.h"
#include "Engine/FRenderTarget.h"
#include "Engine/FViewportInput.h"
#include "Engine/FViewportClient.h"
#include "RenderCore/IRendererModule.h"

FSceneViewport::FSceneViewport(IRendererModule& Renderer, FViewportClient& Client)
	: mRenderer{&Renderer},
	  mClient{&Client} {
}

FSceneViewport::~FSceneViewport() = default;

bool FSceneViewport::Resize(Uint32 Width, Uint32 Height) {
    if (mTarget == nullptr) {
        mTarget = mRenderer->CreateRenderTarget(Width, Height);

        return mTarget != nullptr;
    }

    if (Width == mTarget->GetWidth() && Height == mTarget->GetHeight()) {
        return true;
    }

    return mRenderer->ResizeRenderTarget(*mTarget, Width, Height);
}

void FSceneViewport::Input(const FViewportInput& Input, float DeltaTime) {
    mClient->Input(Input, GetSize(), DeltaTime);
}

void FSceneViewport::Draw() {
    if (mTarget != nullptr) {
        mClient->Draw(*this);
    }
}

FRenderTarget* FSceneViewport::GetRenderTarget() const {
    return mTarget.get();
}

FVector2 FSceneViewport::GetSize() const {
    return mTarget != nullptr ? FVector2{static_cast<float>(mTarget->GetWidth()), static_cast<float>(mTarget->GetHeight())} : FVector2{};
}
