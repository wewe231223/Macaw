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

namespace Editor {

    Editor::FPlayInEditorSession::FPlayInEditorSession(Engine::UEngine* Engine, Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport)
		: mEngine{Engine},
		  mContext{Context},
		  mViewport{Viewport},
		  mInspectionContext{Engine},
		  mGameClient{Engine} {
    }

    Editor::FPlayInEditorSession::~FPlayInEditorSession() {
        Stop();
    }

    bool Editor::FPlayInEditorSession::CanStart() const {
        return mEngine != nullptr && mContext != nullptr && mViewport != nullptr && !IsActive() && !mEngine->IsDispatching() && !mContext->IsChangingState() && mContext->CanEditWorld() && mContext->GetWorld()->GetPersistentLevel() != nullptr && mContext->GetWorld()->GetActiveCamera() != nullptr && !mViewport->GetClient()->GetGizmo().IsDragging();
    }

    bool Editor::FPlayInEditorSession::Start() {
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

        Engine::UWorld* Source{mContext->GetWorld()};
        Core::TArray<std::pair<const CoreUObject::UObject*, CoreUObject::FObjectHandle>> Originals{{Source, Source->GetHandle()}, {Source->GetPersistentLevel(), Source->GetPersistentLevel()->GetHandle()}};

        for (CoreUObject::FObjectHandle Handle : mContext->GetActorHandles()) {
            const Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(mContext->ResolveObject(Handle))};
            Originals.push_back({Actor, Handle});

            for (const auto& Component : Actor->GetComponents()) {
                Originals.push_back({Component.get(), Component->GetHandle()});
            }
        }

        Core::FString Error{};
        CoreUObject::FObjectDuplicationContext Duplication{};
        mPlayWorld = mEngine->DuplicateWorld(mContext->GetWorldHandle(), Engine::EWorldType::PIE, Error, &Duplication);

        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        bool Ready{World != nullptr && !mStopRequested && mInspectionContext.SetWorld(mPlayWorld) && mGameClient.SetWorld(mPlayWorld)};

        if (Ready) {
            for (const auto& [Original, Handle] : Originals) {
                CoreUObject::UObject* Duplicate{Duplication.FindDuplicate(Original)};

                if (Duplicate != nullptr && mContext->ResolveObject(Handle) == Original) {
                    mObjectPairs.push_back({Handle, Duplicate->GetHandle()});
                }
            }

            Core::TArray<CoreUObject::FObjectHandle> Selection{};

            for (CoreUObject::FObjectHandle Handle : mContext->GetSelectedActors()) {
                const CoreUObject::FObjectHandle Copy{FindPlayObject(Handle)};

                if (Copy.IsValid()) {
                    Selection.push_back(Copy);
                }
            }

            mInspectionContext.SetSelectedActors(Selection);

            if (const CoreUObject::UObject* Selected{mContext->GetSelectedObject()}) {
                const CoreUObject::FObjectHandle Copy{FindPlayObject(Selected->GetHandle())};

                if (Selection.size() <= 1 && Copy.IsValid()) {
                    mInspectionContext.SelectObject(Copy);
                }
            }

            Ready = mEngine->SetCurrentWorld(mPlayWorld) && World->BeginPlay() && World->GetPlayState() == Engine::EWorldPlayState::Playing && World->GetActiveCamera() != nullptr && !mStopRequested;
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

    bool Editor::FPlayInEditorSession::Stop() {
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

        const Engine::FWorldHandle Restore{mEngine->GetWorld(mPreviousWorld) != nullptr ? mPreviousWorld : mContext->GetWorldHandle()};

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

    bool Editor::FPlayInEditorSession::SetPaused(bool Paused) {
        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && World->SetPaused(Paused);
    }

    bool Editor::FPlayInEditorSession::Step(float DeltaTime) {
        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && std::isfinite(DeltaTime) && DeltaTime > 0.0f && World->Step(DeltaTime);
    }

    void Editor::FPlayInEditorSession::Update() {
        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        if (IsActive() && !mTransitioning && !mEngine->IsDispatching() && (mStopRequested || World == nullptr || World->GetPlayState() == Engine::EWorldPlayState::Stopped || World->GetActiveCamera() == nullptr)) {
            if (World != nullptr && World->GetActiveCamera() == nullptr) {
                mContext->AddLog("PIE stopped because its active camera was removed");
            }

            Stop();
        }
    }

    bool Editor::FPlayInEditorSession::IsActive() const {
        return mTransitioning || mPlayWorld.IsValid();
    }

    bool Editor::FPlayInEditorSession::IsPaused() const {
        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        return World != nullptr && World->GetPlayState() == Engine::EWorldPlayState::Paused;
    }

    Engine::FWorldHandle Editor::FPlayInEditorSession::GetWorldHandle() const {
        return mPlayWorld;
    }

    Editor::FEditorContext* Editor::FPlayInEditorSession::GetInspectionContext() {
        return &mInspectionContext;
    }

    Engine::FGameViewportClient* Editor::FPlayInEditorSession::GetViewportClient() {
        return &mGameClient;
    }

    CoreUObject::FObjectHandle Editor::FPlayInEditorSession::FindPlayObject(CoreUObject::FObjectHandle Original) const {
        for (const auto& [Source, Duplicate] : mObjectPairs) {
            if (Source == Original && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Duplicate) != nullptr) {
                return Duplicate;
            }
        }

        return {};
    }

    CoreUObject::FObjectHandle Editor::FPlayInEditorSession::FindEditorObject(CoreUObject::FObjectHandle Duplicate) const {
        for (const auto& [Source, Copy] : mObjectPairs) {
            if (Copy == Duplicate && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Copy) != nullptr) {
                return Source;
            }
        }

        return {};
    }

}
