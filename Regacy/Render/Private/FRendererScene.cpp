#include "pch.h"
#include "FRendererScene.h"
#include "Render/Renderer.h"
#include "RenderCore/FSceneUpdateBatch.h"

FRendererScene::FRendererScene(FRenderer& Renderer, FSceneHandle Handle)
	: mRenderer(&Renderer),
	  mHandle(Handle),
	  mRenderScene(std::make_unique<FRenderScene>(Handle.mId)) {
}

FSceneHandle FRendererScene::GetHandle() const {
    return mHandle;
}

bool FRendererScene::ApplyUpdates(FSceneUpdateBatch& Updates) {
    return mRenderer != nullptr && Updates.mSceneHandle == mHandle && mRenderer->ApplySceneUpdates(Updates);
}

void FRendererScene::Release() {
    if (mRenderer != nullptr) {
        mRenderer->ReleaseScene(mHandle);
    }
}

FRenderScene& FRendererScene::GetRenderScene() {
    return *mRenderScene;
}

const FRenderScene& FRendererScene::GetRenderScene() const {
    return *mRenderScene;
}

void FRendererScene::Reset(Uint64 Generation) {
    mHandle.mGeneration = Generation;
    ResetRenderData();
}

void FRendererScene::ResetRenderData() {
    mRenderScene = std::make_unique<FRenderScene>(mHandle.mId);
}

void FRendererScene::Detach() {
    mRenderer = nullptr;
    mHandle = {};
    mRenderScene.reset();
}
