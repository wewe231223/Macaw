/**
 * @file FAnimationViewerContent.cpp
 * @brief 미리보기 월드에서 스켈레탈 메시와 단일 시퀀스를 재생하는 뷰어 콘텐츠입니다.
 * @ingroup Viewer
 */

#include "Viewer/FAnimationViewerContent.h"

#include "Asset/Animation/UAnimSequence.h"
#include "Asset/FAssetManager.h"
#include "Asset/USkeletalMesh.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/ASkeletalMeshActor.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/Animation/UAnimSingleNodeInstance.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/FLightSceneProxy.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FWorldContext.h"
#include "Engine/UWorld.h"
#include "RenderCore/IRendererModule.h"

#include <algorithm>
#include <cmath>
#include <d3d11.h>

using namespace Viewer;

FAnimationViewerContent::FAnimationViewerContent(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
	: mRenderer{Renderer},
	  mAssets{Assets} {
}

FAnimationViewerContent::~FAnimationViewerContent() = default;

bool FAnimationViewerContent::SetMesh(Asset::USkeletalMesh* Mesh, Asset::USkeleton* Skeleton, Core::FString& Error) {
    Error.clear();

    if (Mesh == nullptr) {
        mComponent = nullptr;
        mScene.reset();
        mBounds = {};

        return true;
    }

    if (Skeleton == nullptr) {
        Error = "Select a skeletal mesh for the preview.";

        return false;
    }

    auto Scene{std::make_unique<Viewer::FPreviewScene>(mRenderer, mAssets)};

    if (!Scene->Initialize(Error)) {
        return false;
    }

    Engine::UWorld* World{Scene->GetWorld()};

    auto* Actor{CoreUObject::Cast<Engine::ASkeletalMeshActor>(World->SpawnActor(Engine::ASkeletalMeshActor::StaticClass()))};
    Engine::USkeletalMeshComponent* Component{Actor != nullptr ? Actor->GetSkeletalMeshComponent() : nullptr};

    if (Component == nullptr || !Component->SetPreviewAssets(Mesh, Skeleton) || Component->GetSkeletalMesh() == nullptr || Component->GetSkeleton() == nullptr || !Component->GetPose().GetRevision().IsValid()) {
        Error = "Cannot prepare this mesh and skeleton. Select a compatible skeleton.";

        return false;
    }

    if (mRenderer != nullptr && !Component->GetSkeletalMesh()->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice()))) {
        Error = "Cannot create the skeletal mesh preview resources.";

        return false;
    }

    const double Position{GetPosition()};
    const bool Playing{IsPlaying()};

    mScene = std::move(Scene);
    mComponent = Component;
    mComponent->SetApplyRootMotionToActor(false);
    SetAnimation(mSequence);
    Seek(Position);

    if (Playing) {
        Play();
    }

    return true;
}

void FAnimationViewerContent::SetAnimation(const Asset::UAnimSequence* Sequence) {
    mSequence = Sequence;

    if (mComponent == nullptr) {
        return;
    }

    mComponent->PlayAnimation(Sequence, mLooping);
    GetInstance()->InitializeAnimation(mComponent);
    mComponent->SetPlayRate(mPlayRate);
    Pause();
    SetPreviewMode(mPreviewMode);
    Seek(0.0);
}

void FAnimationViewerContent::Play() {
    Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    if (Instance == nullptr || mSequence == nullptr) {
        return;
    }

    if ((mPlayRate >= 0.0f && GetPosition() >= mSequence->GetPlayLength()) || (mPlayRate < 0.0f && GetPosition() <= 0.0)) {
        Instance->SetPosition(mPlayRate < 0.0f ? mSequence->GetPlayLength() : 0.0);
    }

    Instance->SetPlayRate(mPlayRate);
    Instance->Play(mLooping);
}

void FAnimationViewerContent::Pause() {
    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->Stop();
    }
}

void FAnimationViewerContent::Seek(double Time) {
    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->Stop();
        Instance->SetPosition(Time);
        RefreshPose();
    }
}

void FAnimationViewerContent::Tick(float DeltaTime) {
    Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    if (Instance == nullptr || !Instance->IsPlaying()) {
        return;
    }

    Instance->SetLooping(mLooping);
    mComponent->TickAnimation(std::isfinite(DeltaTime) ? std::max(DeltaTime, 0.0f) : 0.0f);

    if (!mLooping && mSequence != nullptr && ((mPlayRate > 0.0f && GetPosition() >= mSequence->GetPlayLength()) || (mPlayRate < 0.0f && GetPosition() <= 0.0))) {
        Instance->Stop();
    }

    UpdateRootMotion();
    SynchronizeScene();
}

void FAnimationViewerContent::RefreshPose() {
    Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    if (Instance == nullptr) {
        return;
    }

    Instance->SetLooping(false);
    mComponent->TickAnimation(0.0f);
    Instance->SetLooping(mLooping);
    UpdateRootMotion();
    SynchronizeScene();
}

void FAnimationViewerContent::SetLooping(bool Looping) {
    mLooping = Looping;

    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->SetLooping(Looping);
    }
}

void FAnimationViewerContent::SetPlayRate(float PlayRate) {
    mPlayRate = std::isfinite(PlayRate) ? std::clamp(PlayRate, -4.0f, 4.0f) : 1.0f;

    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->SetPlayRate(mPlayRate);
    }
}

void FAnimationViewerContent::SetPreviewMode(Core::Int32 Mode) {
    mPreviewMode = std::clamp(Mode, 0, 2);

    if (mComponent != nullptr) {
        mComponent->SetRootMotionMode(mPreviewMode == 0 ? Engine::ERootMotionMode::NoRootMotionExtraction : mPreviewMode == 1 ? Engine::ERootMotionMode::IgnoreRootMotion : Engine::ERootMotionMode::RootMotionFromEverything);
        RefreshPose();
    }
}

void FAnimationViewerContent::UpdateRootMotion() {
    Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    if (Instance == nullptr || mComponent->GetOwner() == nullptr) {
        return;
    }

    mComponent->GetOwner()->SetActorTransform({});

    if (mPreviewMode == 2 && mSequence != nullptr) {
        const Core::FTransform Motion{Instance->ExtractSequenceRootMotion(*mSequence, 0.0, GetPosition(), false)};
        mComponent->GetOwner()->SetActorTransform(Engine::FAnimationRuntime::ApplyRootMotionToTransform(Motion, mComponent->GetTransform(), {}));
    }

    Instance->ResetRootMotion();
}

void FAnimationViewerContent::SynchronizeScene() {
    const Core::FBox& BindBounds{mComponent->GetSkeletalMesh()->GetBounds()};
    const Core::FMatrix World{mComponent->GetWorldMatrix()};

    mBounds = BindBounds.Transformed(World);

    for (const Core::FMatrix& Skin : mComponent->GetPose().GetSkinMatrices()) {
        const Core::FBox Bounds{BindBounds.Transformed(Skin * World)};

        mBounds.mMinimum = mBounds.mMinimum.Min(Bounds.mMinimum);
        mBounds.mMaximum = mBounds.mMaximum.Max(Bounds.mMaximum);
    }

    GetWorld()->SendAllEndOfFrameUpdates();

    if (Engine::FSceneInterface* Scene{GetWorld()->GetScene()}) {
        Scene->UpdatePrimitiveTransform(mComponent->GetGuid(), World, mBounds);
    }
}

const Core::FBox& FAnimationViewerContent::GetBounds() const {
    return mBounds;
}

Engine::UAnimSingleNodeInstance* FAnimationViewerContent::GetInstance() const {
    return mComponent != nullptr ? CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mComponent->GetAnimInstance()) : nullptr;
}

bool FAnimationViewerContent::IsReady() const {
    return mComponent != nullptr && mSequence != nullptr;
}

bool FAnimationViewerContent::IsPlaying() const {
    const Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    return Instance != nullptr && Instance->IsPlaying();
}

double FAnimationViewerContent::GetPosition() const {
    const Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    return Instance != nullptr ? Instance->GetPosition() : 0.0;
}

float FAnimationViewerContent::GetPlayRate() const {
    return mPlayRate;
}

bool FAnimationViewerContent::IsLooping() const {
    return mLooping;
}

Core::Int32 FAnimationViewerContent::GetPreviewMode() const {
    return mPreviewMode;
}

Engine::UWorld* FAnimationViewerContent::GetWorld() const {
    return mScene != nullptr ? mScene->GetWorld() : nullptr;
}

Engine::USkeletalMeshComponent* FAnimationViewerContent::GetComponent() const {
    return mComponent;
}

Engine::FSceneInterface* FAnimationViewerContent::GetScene() const {
    return mScene != nullptr ? mScene->GetScene() : nullptr;
}

Viewer::EViewerPrepareResult FAnimationViewerContent::PrepareCapture(const Viewer::FViewerCaptureState& State, Core::FString& Error) {
    if (Viewer::IViewerContent::PrepareCapture(State, Error) != Viewer::EViewerPrepareResult::Ready) {
        return Viewer::EViewerPrepareResult::Failed;
    }

    Seek(State.GetTime());

    return Viewer::EViewerPrepareResult::Ready;
}
