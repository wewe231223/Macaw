/**
 * @file EditorEngine.cpp
 * @brief 엔진, UI 독립 편집 컨텍스트와 UI 호스트의 수명을 연결합니다.
 * @ingroup Editor
 */

#include "Editor/EditorEngine.h"
#include "Core/FilePath.h"
#include "Core/LoadingStatus.h"
#include "FEditorUI.h"
#include "Engine/UWorld.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ACameraActor.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FProject.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace Editor {

    /**
     * @brief 편집 상태와 UI의 비소유 연결 및 새 액터 배치 변환 제공 경로를 구성합니다.
     */
    Editor::UEditorEngine::UEditorEngine()
		: mContext{this},
		  mViewport{&mContext},
		  mPlaySession{this, &mContext, &mViewport},
		  mUI{std::make_unique<Editor::FEditorUI>(&mContext, &mViewport, &mPlaySession)} {
        mContext.SetPlacementProvider([this]() {
            return Core::FTransform{mUI->GetActiveViewport()->GetClient()->GetPlacementLocation(), {}, {1.0f, 1.0f, 1.0f}};
        });

        mContext.SetViewportCommandHandler([this](const Core::FString& Command, Core::FString& Output) {
            std::istringstream Input{Command};
            Core::FString Verb{};
            Core::FString State{};
            Core::FString Extra{};
            Input >> Verb >> State >> Extra;
            std::transform(Verb.begin(), Verb.end(), Verb.begin(), [](unsigned char Character) {
                return static_cast<char>(std::tolower(Character));
            });

            std::transform(State.begin(), State.end(), State.begin(), [](unsigned char Character) {
                return static_cast<char>(std::tolower(Character));
            });

            if (Verb == "showflag.shadows" || Verb == "showflag.fog" || Verb == "showflag.fxaa") {
                if (!Extra.empty() || (!State.empty() && State != "on" && State != "off" && State != "0" && State != "1")) {
                    Output = "Usage: " + Verb + " [on/off]";
                    return true;
                }

                Editor::FEditorViewportClient* Client{mUI->GetActiveViewport()->GetClient()};
                Engine::FEngineShowFlags Flags{Client->GetShowFlags()};
                bool& Enabled{Verb == "showflag.fog" ? Flags.mFog : Verb == "showflag.fxaa" ? Flags.mFXAA : Flags.mShadows};
                Enabled = State.empty() ? !Enabled : State == "on" || State == "1";
                Client->SetShowFlags(Flags);
                Output = Verb + (Enabled ? ": on" : ": off");

                return true;
            }

            return mUI->GetActiveViewport()->GetStatistics()->ExecuteCommand(Command, Output);
        });

        mContext.SetCameraHandlers([this]() {
            return mViewport.GetClient()->GetPerspectiveCamera();
        }, [this](const Engine::FEditorCameraState& State) {
            mViewport.GetClient()->SetPerspectiveCamera(State);
        });
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::UEditorEngine::~UEditorEngine() {
        Shutdown();
    }

    /**
     * @brief 헤드리스 엔진과 편집 월드 연결을 준비합니다.
     */
    bool Editor::UEditorEngine::Initialize() {
        if (!Engine::UEngine::Initialize()) {
            return false;
        }

        StaticClass();

        if (!mContext.SetWorld(GetWorldHandle())) {
            Engine::UEngine::Shutdown();
            return false;
        }

        return true;
    }

    /**
     * @brief 렌더러가 연결된 엔진과 편집 월드를 준비합니다.
     */
    bool Editor::UEditorEngine::Initialize(RenderCore::IRendererModule* Renderer) {
        if (!Engine::UEngine::Initialize(Renderer)) {
            return false;
        }

        StaticClass();

        if (!mContext.SetWorld(GetWorldHandle())) {
            Engine::UEngine::Shutdown();
            return false;
        }

        return true;
    }

    /**
     * @brief 엔진, ImGui 백엔드, 뷰포트와 초기 장면을 준비합니다.
     */
    bool Editor::UEditorEngine::InitializeEditor(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform) {
        if (Renderer == nullptr || Platform == nullptr || !Initialize(Renderer)) {
            return false;
        }

        if (!mViewport.Initialize(Renderer) || !mUI->Initialize(Renderer, Platform)) {
            Shutdown();
            return false;
        }

        if (GetProject() != nullptr) {
            const Core::FString& Startup{GetProject()->GetStartupScene()};

            Core::LoadingStatus::BeginStage("Loading startup scene...", 0.82f);

            if (!mContext.LoadScene(Core::FilePath::FromUtf8(Startup))) {
                Shutdown();
                return false;
            }
        }

        SynchronizeWorld();

        return true;
    }

    /**
     * @brief 안전한 엔진 경계에서 UI, 편집 상태와 월드를 역순으로 정리합니다.
     */
    void Editor::UEditorEngine::Shutdown() {
        if (IsDispatching()) {
            Engine::UEngine::Shutdown();
            return;
        }

        mPlaySession.Stop();

        if (mPlaySession.IsActive()) {
            return;
        }

        mUI->Shutdown();
        mViewport.Shutdown();
        mContext.Reset();
        Engine::UEngine::Shutdown();
    }

    /**
     * @brief UI 입력, 월드 실행과 렌더 동기화를 순서대로 진행합니다.
     */
    void Editor::UEditorEngine::TickFrame(float DeltaTime) {
        if (IsDispatching()) {
            return;
        }

        mPlaySession.Update();

        const bool HasUI{mUI->BeginFrame(DeltaTime)};

        mPlaySession.Update();
        Engine::UEngine::Tick(DeltaTime, !mPlaySession.IsActive());
        mPlaySession.Update();
        SynchronizeWorld();

        if (HasUI) {
            mUI->EndFrame(DeltaTime);
        }
    }

    Editor::FPlayInEditorSession* Editor::UEditorEngine::GetPlaySession() {
        return &mPlaySession;
    }

    /**
     * @brief 공유 편집 컨텍스트의 비소유 주소를 반환합니다.
     */
    Editor::FEditorContext* Editor::UEditorEngine::GetEditorContext() {
        return &mContext;
    }

    /**
     * @brief 에디터 뷰포트 클라이언트를 반환합니다.
     */
    Editor::FEditorViewportClient* Editor::UEditorEngine::GetViewportClient() {
        return mViewport.GetClient();
    }

    /**
     * @brief 독립 뷰포트가 소유한 렌더 타깃을 반환합니다.
     */
    Engine::FRenderTarget* Editor::UEditorEngine::GetViewportTarget() const {
        return mViewport.GetRenderTarget();
    }

    /**
     * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
     */
    bool Editor::UEditorEngine::WantsExit() const {
        return mUI->WantsExit();
    }

    /**
     * @brief 엔진이 소유한 기본 독립 에디터 뷰포트의 비소유 주소를 반환합니다.
     */
    Editor::FEditorViewport* Editor::UEditorEngine::GetEditorViewport() {
        return &mViewport;
    }

}
