#include "FAnimationPreviewScene.h"

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

using namespace Editor;

FAnimationPreviewScene::FAnimationPreviewScene(RenderCore::IRendererModule* Renderer, Asset::FAssetManager* Assets)
	: mRenderer{Renderer},
	  mAssets{Assets} {
}

FAnimationPreviewScene::~FAnimationPreviewScene() = default;

bool FAnimationPreviewScene::SetMesh(const Core::FString& MeshPath, const Core::FString& SkeletonPath, Core::FString& Error) {
    Error.clear();

    if (MeshPath.empty()) {
        mComponent = nullptr;
        mWorldContext.reset();
        mBounds = {};

        return true;
    }

    if (mAssets == nullptr) {
        Error = "Select a skeletal mesh for the preview.";

        return false;
    }

    auto Context{std::make_unique<Engine::FWorldContext>(Engine::EWorldType::EditorPreview)};
    Engine::UWorld* World{Context->GetWorld()};
    World->SetAssets(mAssets);

    if (mRenderer != nullptr) {
        auto Scene{mRenderer->AllocateScene()};

        if (Scene == nullptr || !World->AttachScene(std::move(Scene))) {
            Error = "Cannot create the animation preview scene.";

            return false;
        }

        Engine::FLightSceneProxy Ambient{};
        Ambient.mIntensity = 0.5f;
        World->GetScene()->AddLight(Ambient);

        Engine::FLightSceneProxy Directional{};
        Directional.mType = Engine::ELightType::Directional;
        Directional.mDirection = Core::FVector{0.35f, 0.6f, -1.0f}.Normalized();
        Directional.mIntensity = 0.8f;
        World->GetScene()->AddLight(Directional);
    }

    auto* Actor{CoreUObject::Cast<Engine::ASkeletalMeshActor>(World->SpawnActor(Engine::ASkeletalMeshActor::StaticClass()))};
    Engine::USkeletalMeshComponent* Component{Actor != nullptr ? Actor->GetSkeletalMeshComponent() : nullptr};

    if (Component == nullptr || !Component->SetMeshPath(MeshPath) || !Component->SetSkeletonPath(SkeletonPath) || Component->GetSkeletalMesh() == nullptr || Component->GetSkeleton() == nullptr || !Component->GetPose().GetRevision().IsValid()) {
        Error = "Cannot prepare this mesh and skeleton. Select a compatible skeleton.";

        return false;
    }

    if (mRenderer != nullptr && !Component->GetSkeletalMesh()->CreateResources(static_cast<ID3D11Device*>(mRenderer->GetNativeDevice()))) {
        Error = "Cannot create the skeletal mesh preview resources.";

        return false;
    }

    const double Position{GetPosition()};
    const bool Playing{IsPlaying()};

    mWorldContext = std::move(Context);
    mComponent = Component;
    mComponent->SetApplyRootMotionToActor(false);
    SetAnimation(mSequence);
    Seek(Position);

    if (Playing) {
        Play();
    }

    return true;
}

void FAnimationPreviewScene::SetAnimation(const Asset::UAnimSequence* Sequence) {
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

void FAnimationPreviewScene::Play() {
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

void FAnimationPreviewScene::Pause() {
    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->Stop();
    }
}

void FAnimationPreviewScene::Seek(double Time) {
    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->Stop();
        Instance->SetPosition(Time);
        RefreshPose();
    }
}

void FAnimationPreviewScene::Tick(float DeltaTime) {
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

void FAnimationPreviewScene::RefreshPose() {
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

void FAnimationPreviewScene::SetLooping(bool Looping) {
    mLooping = Looping;

    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->SetLooping(Looping);
    }
}

void FAnimationPreviewScene::SetPlayRate(float PlayRate) {
    mPlayRate = std::isfinite(PlayRate) ? std::clamp(PlayRate, -4.0f, 4.0f) : 1.0f;

    if (Engine::UAnimSingleNodeInstance* Instance{GetInstance()}) {
        Instance->SetPlayRate(mPlayRate);
    }
}

void FAnimationPreviewScene::SetPreviewMode(Core::Int32 Mode) {
    mPreviewMode = std::clamp(Mode, 0, 2);

    if (mComponent != nullptr) {
        mComponent->SetRootMotionMode(mPreviewMode == 0 ? Engine::ERootMotionMode::NoRootMotionExtraction : mPreviewMode == 1 ? Engine::ERootMotionMode::IgnoreRootMotion : Engine::ERootMotionMode::RootMotionFromEverything);
        RefreshPose();
    }
}

void FAnimationPreviewScene::UpdateRootMotion() {
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

void FAnimationPreviewScene::SynchronizeScene() {
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

const Core::FBox& FAnimationPreviewScene::GetBounds() const {
    return mBounds;
}

Engine::UAnimSingleNodeInstance* FAnimationPreviewScene::GetInstance() const {
    return mComponent != nullptr ? CoreUObject::Cast<Engine::UAnimSingleNodeInstance>(mComponent->GetAnimInstance()) : nullptr;
}

bool FAnimationPreviewScene::IsReady() const {
    return mComponent != nullptr && mSequence != nullptr;
}

bool FAnimationPreviewScene::IsPlaying() const {
    const Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    return Instance != nullptr && Instance->IsPlaying();
}

double FAnimationPreviewScene::GetPosition() const {
    const Engine::UAnimSingleNodeInstance* Instance{GetInstance()};

    return Instance != nullptr ? Instance->GetPosition() : 0.0;
}

float FAnimationPreviewScene::GetPlayRate() const {
    return mPlayRate;
}

bool FAnimationPreviewScene::IsLooping() const {
    return mLooping;
}

Core::Int32 FAnimationPreviewScene::GetPreviewMode() const {
    return mPreviewMode;
}

Engine::UWorld* FAnimationPreviewScene::GetWorld() const {
    return mWorldContext != nullptr ? mWorldContext->GetWorld() : nullptr;
}

Engine::USkeletalMeshComponent* FAnimationPreviewScene::GetComponent() const {
    return mComponent;
}
