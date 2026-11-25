/**
 * @file FPlayInEditorSession.cpp
 * @brief 편집 월드 복제와 PIE 실행 세션의 수명 관리에 대한 구현입니다.
 * @ingroup Editor
 */

#include "Editor/FPlayInEditorSession.h"
#include "Core/Literals/UnitLiterals.h"
#include "Editor/FEditorViewport.h"
#include "FPlayInEditorWindow.h"
#include "Engine/Engine.h"
#include "Engine/FProject.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/Cast.h"
#include <cmath>

using namespace Core::Literals;

namespace Editor {

    constinit const Core::FTime Editor::FPlayInEditorSession::mDefaultStep{1.0f / 60_Hz};

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

    const FPlayInEditorSession::FOptions& FPlayInEditorSession::GetOptions() const {
        return mOptions;
    }

    bool FPlayInEditorSession::SetOptions(const FOptions& Options) {
        if (IsActive() || Options.mWindowWidth < 320 || Options.mWindowWidth > 7680 || Options.mWindowHeight < 180 || Options.mWindowHeight > 4320) {
            return false;
        }

        mOptions = Options;

        return true;
    }

    bool FPlayInEditorSession::IsPlayingInNewWindow() const {
        return IsActive() && mPlayWindow != nullptr;
    }

    void FPlayInEditorSession::DrawPlayWindow() {
        if (mPlayWindow != nullptr && !mPlayWindow->Draw()) {
            mContext->AddLog("PIE window could not present its frame");
            mStopRequested = true;
        }
    }

    /**
     * @brief 편집 월드와 뷰포트 상태가 PIE 시작 조건을 만족하는지 검사합니다.
     */
    bool Editor::FPlayInEditorSession::CanStart(EMode Mode) const {
        return mEngine != nullptr && mContext != nullptr && mViewport != nullptr && !IsActive() && !mEngine->IsDispatching() && !mContext->IsChangingState() && !mContext->IsSceneEditActive() && mContext->CanEditWorld() && mContext->GetWorld()->GetPersistentLevel() != nullptr && (Mode == EMode::Simulate || mContext->GetWorld()->GetActiveCamera() != nullptr) && !mViewport->GetClient()->GetGizmo().IsDragging();
    }

    /**
     * @brief 편집 월드를 복제하여 PIE를 시작하고 뷰포트 입력과 조회 대상을 연결합니다.
     */
    bool Editor::FPlayInEditorSession::Start(Core::FInput* Input, EMode Mode) {
        const bool Playing{Mode == EMode::Play};

        if (!CanStart(Mode)) {
            if (Playing && !IsActive() && mContext != nullptr && mContext->GetWorld() != nullptr && mContext->GetWorld()->GetActiveCamera() == nullptr) {
                mContext->AddLog("PIE requires an active world camera");
            }

            return false;
        }

        mContext->RestorePreview();

        mMode = Mode;
        mTransitioning = true;
        mStopRequested = false;
        mContext->mPlayLocked = true;
        mPreviousWorld = mEngine->GetWorldHandle();

        const Editor::FEditorViewportClient* ViewportClient{mViewport->GetClient()};
        mGameClient.SetDepthOfFieldShowFlag(ViewportClient->GetShowFlags().mDepthOfField);
        mGameClient.SetPhysicsBodiesView(ViewportClient->GetViewMode() == Engine::EViewMode::PhysicsBodies, ViewportClient->GetLineThickness());

        Engine::UWorld* Source{mContext->GetWorld()};
        Core::TArray<std::pair<const CoreUObject::UObject*, CoreUObject::TObjectRef<CoreUObject::UObject>>> Originals{{Source, CoreUObject::MakeObjectRef(Source)}, {Source->GetPersistentLevel(), CoreUObject::MakeObjectRef(Source->GetPersistentLevel())}};

        for (CoreUObject::TObjectRef<CoreUObject::UObject> Handle : mContext->GetActorRefs()) {
            const Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(mContext->ResolveObject(Handle))};

            Originals.push_back({Actor, Handle});

            for (const auto& Component : Actor->GetComponents()) {
                Originals.push_back({Component.get(), CoreUObject::MakeObjectRef(Component)});
            }
        }

        Core::FString Error{};
        CoreUObject::FObjectDuplicationContext Duplication{};

        mPlayWorld = mEngine->DuplicateWorld(mContext->GetWorldHandle(), Engine::EWorldType::PIE, Error, &Duplication);

        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        bool Ready{World != nullptr && !mStopRequested && mInspectionContext.SetWorld(mPlayWorld) && (!Playing || mGameClient.SetWorld(mPlayWorld))};

        if (Ready) {
            for (const auto& [Original, Handle] : Originals) {
                CoreUObject::UObject* Duplicate{Duplication.FindDuplicate(Original)};

                if (Duplicate != nullptr && mContext->ResolveObject(Handle) == Original) {
                    mObjectPairs.push_back({Handle, CoreUObject::MakeObjectRef(Duplicate)});
                }
            }

            Core::TArray<CoreUObject::TObjectRef<Engine::AActor>> Selection{};

            for (CoreUObject::TObjectRef<CoreUObject::UObject> Handle : mContext->GetSelectedActors()) {
                const CoreUObject::TObjectRef<CoreUObject::UObject> Copy{FindPlayObject(Handle)};

                if (Copy.IsValid()) {
                    Selection.push_back(CoreUObject::Cast<Engine::AActor>(Copy.Get()));
                }
            }

            mInspectionContext.SetSelectedActors(Selection);

            if (Playing && mOptions.mStartAtEditorCamera) {
                Engine::UCameraComponent* Camera{World->GetActiveCamera()};
                const Engine::FEditorCameraState EditorCamera{ViewportClient->GetPerspectiveCamera()};

                Ready = Camera != nullptr && Camera->SetTransform(FTransform{EditorCamera.GetLocation(), FQuat{EditorCamera.GetRotation()}, {1.0f, 1.0f, 1.0f}}) && Camera->SetFieldOfView(EditorCamera.GetFieldOfView());
            }

            if (Ready && Playing && mOptions.mNewWindow) {
                mPlayWindow = std::make_unique<Editor::FPlayInEditorWindow>();
                Ready = mPlayWindow->Initialize(mEngine->GetRenderer(), &mGameClient, mOptions.mWindowWidth, mOptions.mWindowHeight, mOptions.mCaptureInput && !mOptions.mStartPaused);

                if (Ready) {
                    Input = &mPlayWindow->GetInput();
                }
            }

            if (CoreUObject::UObject * Selected{mContext->GetSelectedObject()}) {
                const CoreUObject::TObjectRef<CoreUObject::UObject> Copy{FindPlayObject(CoreUObject::MakeObjectRef(Selected))};

                if (Selection.size() <= 1 && Copy.IsValid()) {
                    mInspectionContext.SelectObject(Copy);
                }
            }

            Ready = Ready && mEngine->SetCurrentWorld(mPlayWorld) && World->BeginPlay() && World->GetPlayState() == Engine::EWorldPlayState::Playing && (!Playing || World->GetActiveCamera() != nullptr) && !mStopRequested;

            if (Ready && mOptions.mStartPaused) {
                Ready = World->SetPaused(true);
            }
        }

        if (Ready && Playing) {
            mInput = Input;

            if (mInput != nullptr) {
                mInput->SetGameInputEnabled(false);
            }

            if (mPlayWindow == nullptr) {
                mViewport->SetPlayClient(&mGameClient);

                if (mOptions.mCaptureInput && !mOptions.mStartPaused) {
                    mViewport->CapturePlayInput();
                }
            }
        }

        mTransitioning = false;

        if (Ready) {
            mContext->AddLog(Playing ? "PIE started" : "Simulation started");
        } else {
            mContext->AddLog(Error.empty() ? (Playing ? "PIE could not start" : "Simulation could not start") : Error);

            if (mPlayWorld.IsValid()) {
                Stop();
            } else {
                mMode = EMode::Play;
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

        const bool Simulating{IsSimulating()};

        if (!Simulating) {
            mViewport->SetPlayClient(nullptr);
            mGameClient.SetWorld({});
        }

        mInspectionContext.Reset();

        if (mEngine->GetWorld(mPlayWorld) != nullptr) {
            mEngine->DestroyWorld(mPlayWorld);
        }

        mPlayWindow.reset();

        const Engine::FWorldHandle Restore{mEngine->GetWorld(mPreviousWorld) != nullptr ? mPreviousWorld : mContext->GetWorldHandle()};

        if (mEngine->GetWorld(Restore) != nullptr) {
            mEngine->SetCurrentWorld(Restore);
        }

        mObjectPairs.clear();
        mPlayWorld = {};
        mPreviousWorld = {};
        mMode = EMode::Play;
        mContext->mPlayLocked = false;
        mStopRequested = false;
        mTransitioning = false;
        mContext->AddLog(Simulating ? "Simulation stopped" : "PIE stopped");

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
    bool Editor::FPlayInEditorSession::Step(Core::FTime DeltaTime) {
        Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};

        return !mTransitioning && !mEngine->IsDispatching() && World != nullptr && DeltaTime.IsFinite() && DeltaTime > Core::FTime{} && World->Step(DeltaTime);
    }

    /**
     * @brief PIE 월드의 유효성을 검사하고 지연된 종료 요청을 처리합니다.
     */
    void Editor::FPlayInEditorSession::Update() {
        if (!IsActive() || mTransitioning || mEngine->IsDispatching()) {
            return;
        }

        const Engine::UWorld* World{mEngine->GetWorld(mPlayWorld)};
        const bool Playing{mMode == EMode::Play};
        const bool CameraMissing{Playing && World != nullptr && World->GetActiveCamera() == nullptr};

        if (mStopRequested || World == nullptr || World->GetPlayState() == Engine::EWorldPlayState::Stopped || CameraMissing || (mPlayWindow != nullptr && (!mPlayWindow->IsOpen() || mPlayWindow->GetInput().WasPressed(Core::EKey::Escape)))) {
            if (CameraMissing) {
                mContext->AddLog("PIE stopped because its active camera was removed");
            }

            Stop();

            return;
        }

        if (Playing) {
            const Editor::FEditorViewportClient* ViewportClient{mViewport->GetClient()};
            mGameClient.SetDepthOfFieldShowFlag(ViewportClient->GetShowFlags().mDepthOfField);
            mGameClient.SetPhysicsBodiesView(ViewportClient->GetViewMode() == Engine::EViewMode::PhysicsBodies, ViewportClient->GetLineThickness());
        }

        Engine::FWorldHandle Replacement{};
        Core::FString Error{};
        bool Ready{true};

        mTransitioning = true;

        const bool Changed{mEngine->ProcessSceneTransition(mPlayWorld, Replacement, Error, Playing)};

        if (Changed) {
            mPlayWorld = Replacement;
            mObjectPairs.clear();
            mInspectionContext.Reset();

            Ready = mInspectionContext.SetWorld(mPlayWorld) && (!Playing || mGameClient.SetWorld(mPlayWorld));

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

        if (!Playing) {
            if (mPlayWindow != nullptr) {
                mPlayWindow->ReleaseInput();
            } else {
                mViewport->ReleasePlayInput();
            }
        }

        const bool Captured{Playing && (mPlayWindow != nullptr ? mPlayWindow->IsInputCaptured() : mViewport->IsPlayInputCaptured())};

        mInput->SetGameInputEnabled(Playing && (mPlayWindow != nullptr || AllowInput) && Captured);

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

    bool Editor::FPlayInEditorSession::IsSimulating() const {
        return IsActive() && mMode == EMode::Simulate;
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
    CoreUObject::TObjectRef<CoreUObject::UObject> Editor::FPlayInEditorSession::FindPlayObject(CoreUObject::TObjectRef<CoreUObject::UObject> Original) const {
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
    CoreUObject::TObjectRef<CoreUObject::UObject> Editor::FPlayInEditorSession::FindEditorObject(CoreUObject::TObjectRef<CoreUObject::UObject> Duplicate) const {
        for (const auto& [Source, Copy] : mObjectPairs) {
            if (Copy == Duplicate && mContext->ResolveObject(Source) != nullptr && mInspectionContext.ResolveObject(Copy) != nullptr) {
                return Source;
            }
        }

        return {};
    }

}
