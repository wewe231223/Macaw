/**
 * @file FPlayInEditorSession.cpp
 * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리에 대한 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FPlayInEditorSession.h"
#include "Editor/FEditorViewport.h"
#include "Engine/Engine.h"
#include "Engine/FProject.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Script/FScriptInput.h"
#include "Script/FScriptRuntime.h"
#include <cmath>

namespace Editor {

    /**
     * @brief 전달받은 설정과 의존성으로 FPlayInEditorSession의 초기 상태를 구성합니다.
     */
    Editor::FPlayInEditorSession::FPlayInEditorSession(Engine::UEngine* Engine, Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport)
		: mEngine{Engine},
		  mContext{Context},
		  mViewport{Viewport},
		  mInspectionContext{Engine},
		  mGameClient{Engine} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
     */
    Editor::FPlayInEditorSession::~FPlayInEditorSession() {
        Stop();
    }

    /**
     * @brief 편집 월드와 뷰포트 상태가 PIE 시작 조건을 만족하는지 검사합니다.
     */
    bool Editor::FPlayInEditorSession::CanStart() const {
        return mEngine != nullptr && mContext != nullptr && mViewport != nullptr && !IsActive() && !mEngine->IsDispatching() && !mContext->IsChangingState() && !mContext->IsSceneEditActive() && mContext->CanEditWorld() && mContext->GetWorld()->GetPersistentLevel() != nullptr && mContext->GetWorld()->GetActiveCamera() != nullptr && !mViewport->GetClient()->GetGizmo().IsDragging();
    }

    /**
     * @brief 편집 월드를 복제하여 PIE를 시작하고 뷰포트 입력과 조회 대상을 연결합니다.
     */
    bool Editor::FPlayInEditorSession::Start(Core::FInput* Input) {
        if (!CanStart()) {
            if (!IsActive() && mContext != nullptr && mContext->GetWorld() != nullptr && mContext->GetWorld()->GetActiveCamera() == nullptr) {
                mContext->AddLog("PIE requires an active world camera");
            }

            return false;
        }

        mContext->RestorePreview();

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

            if (mEngine->GetProject() != nullptr) {
                Script::FScriptRuntime* Runtime{Script::FScriptRuntime::Get()};
                Runtime->SetLogFunction([this](const Core::FString& Message) {
                    mContext->AddLog("[C#] " + Message);
                });
                Runtime->BeginSession(mEngine->GetProject()->GetRootPath());
                Script::FScriptInput::SetSource(Input);
            }

            Ready = mEngine->SetCurrentWorld(mPlayWorld) && World->BeginPlay() && World->GetPlayState() == Engine::EWorldPlayState::Playing && World->GetActiveCamera() != nullptr && !mStopRequested;
        }

        if (Ready && Input != nullptr) {
            Input->SetGameInputEnabled(false);
        }

        if (Ready) {
            mInput = Input;
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

    /**
     * @brief PIE 실행을 종료하고 복제 월드와 객체 대응표를 정리한 뒤 편집 상태를 복원합니다.
     */
    bool Editor::FPlayInEditorSession::Stop() {
        if (!IsActive()) {
            return false;
        }

        if (mTransitioning || mEngine->IsDispatching()) {
            mStopRequested = true;

            return true;
        }

        mTransitioning = true;

        if (mInput != nullptr) {
            mInput->SetGameInputEnabled(false);
            mInput = nullptr;
        }

        mViewport->SetPlayClient(nullptr);
        mGameClient.SetWorld({});
        mInspectionContext.Reset();

        if (mEngine->GetWorld(mPlayWorld) != nullptr) {
            mEngine->DestroyWorld(mPlayWorld);
        }

        // 월드와 함께 C# 인스턴스가 해제된 뒤에 언로드해야 수집됩니다.
        Script::FScriptInput::SetSource(nullptr);
        Script::FScriptRuntime::Get()->EndSession();
        Script::FScriptRuntime::Get()->SetLogFunction({});

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

    /**
     * @brief PIE 월드의 실행을 일시정지하거나 재개합니다.
     */
    bool Editor::FPlayInEditorSession::SetPaused(bool Paused) {
        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && World->SetPaused(Paused);
    }

    /**
     * @brief 일시정지한 PIE 월드를 지정한 시간만큼 한 번 갱신합니다.
     */
    bool Editor::FPlayInEditorSession::Step(float DeltaTime) {
        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && std::isfinite(DeltaTime) && DeltaTime > 0.0f && World->Step(DeltaTime);
    }

    /**
     * @brief PIE 월드의 유효성을 검사하고 지연된 종료 요청을 처리합니다.
     */
    void Editor::FPlayInEditorSession::Update() {
        if (!IsActive() || mTransitioning || mEngine->IsDispatching()) {
            return;
        }

        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        if (mStopRequested || World == nullptr || World->GetPlayState() == Engine::EWorldPlayState::Stopped || World->GetActiveCamera() == nullptr) {
            if (World != nullptr && World->GetActiveCamera() == nullptr) {
                mContext->AddLog("PIE stopped because its active camera was removed");
            }

            Stop();

            return;
        }

        Engine::FWorldHandle Replacement{};
        Core::FString Error{};
        bool Ready{true};

        mTransitioning = true;

        const bool Changed{mEngine->ProcessSceneTransition(mPlayWorld, Replacement, Error)};

        if (Changed) {
            mPlayWorld = Replacement;
            mObjectPairs.clear();
            mInspectionContext.Reset();

            Ready = mInspectionContext.SetWorld(mPlayWorld) && mGameClient.SetWorld(mPlayWorld);

            if (!Ready && Error.empty()) {
                Error = "Cannot connect the new PIE scene";
            }
        }

        mTransitioning = false;

        if (!Error.empty()) {
            mContext->AddLog("Scene transition failed: " + Error);
        }

        if (Changed && (!Ready || !Error.empty() || mStopRequested)) {
            Stop();
        }
    }

    /**
     * @brief PIE 게임 뷰포트에 프레임 입력과 경과 시간을 전달합니다.
     */
    void Editor::FPlayInEditorSession::ProcessInput(float DeltaTime, bool AllowInput) {
        if (mInput == nullptr) {
            return;
        }

        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        const bool Playing{World != nullptr && World->GetPlayState() == Engine::EWorldPlayState::Playing && !mStopRequested};

        mInput->SetGameInputEnabled(Playing && AllowInput && mViewport->IsPlayInputCaptured());

        if (Playing) {
            mGameClient.ProcessInput(*mInput, DeltaTime);
        }
    }

    /**
     * @brief PIE 월드가 엔진에 유효하게 등록되어 실행 세션이 활성인지 검사합니다.
     */
    bool Editor::FPlayInEditorSession::IsActive() const {
        return mTransitioning || mPlayWorld.IsValid();
    }

    /**
     * @brief PIE 실행 월드의 일시정지 여부를 반환합니다.
     */
    bool Editor::FPlayInEditorSession::IsPaused() const {
        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        return World != nullptr && World->GetPlayState() == Engine::EWorldPlayState::Paused;
    }

    /**
     * @brief PIE 월드의 핸들을 반환합니다.
     */
    Engine::FWorldHandle Editor::FPlayInEditorSession::GetWorldHandle() const {
        return mPlayWorld;
    }

    /**
     * @brief PIE 월드의 선택과 조회에 사용하는 편집 컨텍스트를 반환합니다.
     */
    Editor::FEditorContext* Editor::FPlayInEditorSession::GetInspectionContext() {
        return &mInspectionContext;
    }

    /**
     * @brief PIE 월드의 입력과 장면 뷰를 처리하는 클라이언트를 반환합니다.
     */
    Engine::FGameViewportClient* Editor::FPlayInEditorSession::GetViewportClient() {
        return &mGameClient;
    }

    /**
     * @brief 편집 객체에 대응하는 PIE 복제 객체의 핸들을 조회합니다.
     */
    CoreUObject::FObjectHandle Editor::FPlayInEditorSession::FindPlayObject(CoreUObject::FObjectHandle Original) const {
        for (const auto& [Source, Duplicate] : mObjectPairs) {
            if (Source == Original && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Duplicate) != nullptr) {
                return Duplicate;
            }
        }

        return {};
    }

    /**
     * @brief PIE 복제 객체에 대응하는 원본 편집 객체의 핸들을 조회합니다.
     */
    CoreUObject::FObjectHandle Editor::FPlayInEditorSession::FindEditorObject(CoreUObject::FObjectHandle Duplicate) const {
        for (const auto& [Source, Copy] : mObjectPairs) {
            if (Copy == Duplicate && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Copy) != nullptr) {
                return Source;
            }
        }

        return {};
    }

}
