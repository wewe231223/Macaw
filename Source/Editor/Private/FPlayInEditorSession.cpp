/**
 * @file FPlayInEditorSession.cpp
 * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리에 대한 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FPlayInEditorSession.h"
#include "Editor/FEditorViewport.h"
#include "Engine/Engine.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/Cast.h"
#include <cmath>

FPlayInEditorSession::FPlayInEditorSession(UEngine* Engine, FEditorContext* Context, FEditorViewport* Viewport)
	: mEngine{Engine},
	  mContext{Context},
	  mViewport{Viewport},
	  mInspectionContext{Engine},
	  mGameClient{Engine} {
}

FPlayInEditorSession::~FPlayInEditorSession() {
    Stop();
}

bool FPlayInEditorSession::CanStart() const {
    return mEngine != nullptr && mContext != nullptr && mViewport != nullptr && !IsActive() && !mEngine->IsDispatching() && !mContext->IsChangingState() && mContext->CanEditWorld() && mContext->GetWorld()->GetPersistentLevel() != nullptr && mContext->GetWorld()->GetActiveCamera() != nullptr && !mViewport->GetClient()->GetGizmo().IsDragging();
}

bool FPlayInEditorSession::Start() {
    if (!CanStart()) {
        if (!IsActive() && mContext != nullptr && mContext->GetWorld() != nullptr && mContext->GetWorld()->GetActiveCamera() == nullptr) {
            mContext->AddLog("PIE requires an active world camera");
        }

        return false;
    }

    mTransitioning = true;
    mStopRequested = false;
    mContext->mPlayLocked = true;
    mPreviousWorld = mEngine->GetWorldHandle();

    UWorld* Source{mContext->GetWorld()};
    TArray<std::pair<const UObject*, FObjectHandle>> Originals{{Source, Source->GetHandle()}, {Source->GetPersistentLevel(), Source->GetPersistentLevel()->GetHandle()}};

    for (FObjectHandle Handle : mContext->GetActorHandles()) {
        const AActor* Actor{Cast<AActor>(mContext->ResolveObject(Handle))};
        Originals.push_back({Actor, Handle});

        for (const auto& Component : Actor->GetComponents()) {
            Originals.push_back({Component.get(), Component->GetHandle()});
        }
    }

    FString Error{};
    FObjectDuplicationContext Duplication{};
    mPlayWorld = mEngine->DuplicateWorld(mContext->GetWorldHandle(), EWorldType::PIE, Error, &Duplication);

    UWorld* World{mEngine->GetWorld(mPlayWorld)};
    bool Ready{World != nullptr && !mStopRequested && mInspectionContext.SetWorld(mPlayWorld) && mGameClient.SetWorld(mPlayWorld)};

    if (Ready) {
        for (const auto& [Original, Handle] : Originals) {
            UObject* Duplicate{Duplication.FindDuplicate(Original)};

            if (Duplicate != nullptr && mContext->ResolveObject(Handle) == Original) {
                mObjectPairs.push_back({Handle, Duplicate->GetHandle()});
            }
        }

        TArray<FObjectHandle> Selection{};

        for (FObjectHandle Handle : mContext->GetSelectedActors()) {
            const FObjectHandle Copy{FindPlayObject(Handle)};

            if (Copy.IsValid()) {
                Selection.push_back(Copy);
            }
        }

        mInspectionContext.SetSelectedActors(Selection);

        if (const UObject* Selected{mContext->GetSelectedObject()}) {
            const FObjectHandle Copy{FindPlayObject(Selected->GetHandle())};

            if (Selection.size() <= 1 && Copy.IsValid()) {
                mInspectionContext.SelectObject(Copy);
            }
        }

        Ready = mEngine->SetCurrentWorld(mPlayWorld) && World->BeginPlay() && World->GetPlayState() == EWorldPlayState::Playing && World->GetActiveCamera() != nullptr && !mStopRequested;
    }

    if (Ready) {
        mViewport->SetPlayClient(&mGameClient);
        mContext->AddLog("PIE started");
    }

    mTransitioning = false;

    if (!Ready) {
        mContext->AddLog(Error.empty() ? "PIE could not start" : Error);

        if (mPlayWorld.IsValid()) {
            Stop();
        } else {
            mContext->mPlayLocked = false;
            mPreviousWorld = {};
            mStopRequested = false;
        }
    }

    return Ready;
}

bool FPlayInEditorSession::Stop() {
    if (!IsActive()) {
        return false;
    }

    if (mTransitioning || mEngine->IsDispatching()) {
        mStopRequested = true;
        return true;
    }

    mTransitioning = true;
    mViewport->SetPlayClient(nullptr);
    mGameClient.SetWorld({});
    mInspectionContext.Reset();

    if (mEngine->GetWorld(mPlayWorld) != nullptr) {
        mEngine->DestroyWorld(mPlayWorld);
    }

    const FWorldHandle Restore{mEngine->GetWorld(mPreviousWorld) != nullptr ? mPreviousWorld : mContext->GetWorldHandle()};

    if (mEngine->GetWorld(Restore) != nullptr) {
        mEngine->SetCurrentWorld(Restore);
    }

    mObjectPairs.clear();
    mPlayWorld = {};
    mPreviousWorld = {};
    mContext->mPlayLocked = false;
    mStopRequested = false;
    mTransitioning = false;
    mContext->AddLog("PIE stopped");

    return true;
}

bool FPlayInEditorSession::SetPaused(bool Paused) {
    UWorld* World{mEngine->GetWorld(mPlayWorld)};
    return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && World->SetPaused(Paused);
}

bool FPlayInEditorSession::Step(float DeltaTime) {
    UWorld* World{mEngine->GetWorld(mPlayWorld)};
    return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && std::isfinite(DeltaTime) && DeltaTime > 0.0f && World->Step(DeltaTime);
}

void FPlayInEditorSession::Update() {
    const UWorld* World{mEngine->GetWorld(mPlayWorld)};

    if (IsActive() && !mTransitioning && !mEngine->IsDispatching() && (mStopRequested || World == nullptr || World->GetPlayState() == EWorldPlayState::Stopped || World->GetActiveCamera() == nullptr)) {
        if (World != nullptr && World->GetActiveCamera() == nullptr) {
            mContext->AddLog("PIE stopped because its active camera was removed");
        }

        Stop();
    }
}

bool FPlayInEditorSession::IsActive() const {
    return mTransitioning || mPlayWorld.IsValid();
}

bool FPlayInEditorSession::IsPaused() const {
    const UWorld* World{mEngine->GetWorld(mPlayWorld)};
    return World != nullptr && World->GetPlayState() == EWorldPlayState::Paused;
}

FWorldHandle FPlayInEditorSession::GetWorldHandle() const {
    return mPlayWorld;
}

FEditorContext* FPlayInEditorSession::GetInspectionContext() {
    return &mInspectionContext;
}

FGameViewportClient* FPlayInEditorSession::GetViewportClient() {
    return &mGameClient;
}

FObjectHandle FPlayInEditorSession::FindPlayObject(FObjectHandle Original) const {
    for (const auto& [Source, Duplicate] : mObjectPairs) {
        if (Source == Original && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Duplicate) != nullptr) {
            return Duplicate;
        }
    }

    return {};
}

FObjectHandle FPlayInEditorSession::FindEditorObject(FObjectHandle Duplicate) const {
    for (const auto& [Source, Copy] : mObjectPairs) {
        if (Copy == Duplicate && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Copy) != nullptr) {
            return Source;
        }
    }

    return {};
}
