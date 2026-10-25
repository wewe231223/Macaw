/**
 * @file EditorEngine.cpp
 * @brief 엔진, UI 독립 편집 컨텍스트와 UI 호스트의 수명을 연결합니다.
 * @ingroup Editor
 */

#include "Editor/EditorEngine.h"
#include "FEditorUI.h"
#include "Engine/UWorld.h"
#include "Engine/AStaticMeshActor.h"
#include "Engine/ACameraActor.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/EPrimitiveType.h"

/**
 * @brief 편집 상태와 UI의 비소유 연결 및 새 액터 배치 변환 제공 경로를 구성합니다.
 */
UEditorEngine::UEditorEngine()
	: mContext{this},
	  mViewport{&mContext},
	  mPlaySession{this, &mContext, &mViewport},
	  mUI{std::make_unique<FEditorUI>(&mContext, &mViewport, &mPlaySession)} {
    mContext.SetPlacementProvider([this]() {
        const FSceneView View{mViewport.GetClient()->CalcSceneView({960.0f, 640.0f})};
        return FTransform{View.mLocation + View.mForward * 6.0f, {}, {1.0f, 1.0f, 1.0f}};
    });
    mContext.SetViewportCommandHandler([this](const FString& Command, FString& Output) {
        return mViewport.GetStatistics()->ExecuteCommand(Command, Output);
    });
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UEditorEngine::~UEditorEngine() {
    Shutdown();
}

/**
 * @brief 헤드리스 엔진과 편집 월드 연결을 준비합니다.
 */
bool UEditorEngine::Initialize() {
    if (!UEngine::Initialize()) {
        return false;
    }

    StaticClass();
    if (!mContext.SetWorld(GetWorldHandle())) {
        UEngine::Shutdown();
        return false;
    }

    return true;
}

/**
 * @brief 렌더러가 연결된 엔진과 편집 월드를 준비합니다.
 */
bool UEditorEngine::Initialize(IRendererModule* Renderer) {
    if (!UEngine::Initialize(Renderer)) {
        return false;
    }

    StaticClass();
    if (!mContext.SetWorld(GetWorldHandle())) {
        UEngine::Shutdown();
        return false;
    }

    return true;
}

/**
 * @brief 엔진, ImGui 백엔드, 뷰포트와 초기 장면을 준비합니다.
 */
bool UEditorEngine::InitializeEditor(IRendererModule* Renderer, FWindowsApplication* Platform) {
    if (Renderer == nullptr || Platform == nullptr || !Initialize(Renderer)) {
        return false;
    }

    if (!mViewport.Initialize(Renderer) || !mUI->Initialize(Renderer, Platform)) {
        Shutdown();
        return false;
    }

    AStaticMeshActor* Cube{GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
    AStaticMeshActor* Sphere{GetWorld()->SpawnPrimitive(EPrimitiveType::Sphere)};
    AStaticMeshActor* Plane{GetWorld()->SpawnPrimitive(EPrimitiveType::Plane)};
    auto* Camera{Cast<ACameraActor>(GetWorld()->SpawnActor(ACameraActor::StaticClass()))};

    if (Cube == nullptr || Sphere == nullptr || Plane == nullptr || Camera == nullptr || !Camera->SetActorTransform(FTransform{mViewport.GetClient()->GetCameraLocation(), mViewport.GetClient()->GetCameraRotation().Quaternion(), {1.0f, 1.0f, 1.0f}}) || !GetWorld()->SetActiveCamera(Camera->GetCameraComponent()) || !GetWorld()->SetInputTarget(Camera)) {
        Shutdown();

        return false;
    }

    Sphere->SetActorTransform(FTransform{{0.0f, 2.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}});
    Plane->SetActorTransform(FTransform{{0.0f, 0.0f, -0.55f}, {}, {12.0f, 12.0f, 1.0f}});
    mContext.SelectActor(Cube->GetHandle());

    SynchronizeWorld();

    return true;
}

/**
 * @brief 안전한 엔진 경계에서 UI, 편집 상태와 월드를 역순으로 정리합니다.
 */
void UEditorEngine::Shutdown() {
    if (IsDispatching()) {
        UEngine::Shutdown();
        return;
    }

    mPlaySession.Stop();
    if (mPlaySession.IsActive()) {
        return;
    }
    mUI->Shutdown();
    mViewport.Shutdown();
    mContext.Reset();
    UEngine::Shutdown();
}

/**
 * @brief UI 입력, 월드 실행과 렌더 동기화를 순서대로 진행합니다.
 */
void UEditorEngine::TickFrame(float DeltaTime) {
    if (IsDispatching()) {
        return;
    }
    mPlaySession.Update();
    const bool HasUI{mUI->BeginFrame(DeltaTime)};
    mPlaySession.Update();
    UEngine::Tick(DeltaTime, !mPlaySession.IsActive());
    mPlaySession.Update();
    SynchronizeWorld();
    if (HasUI) {
        mUI->EndFrame(DeltaTime);
    }
}

FPlayInEditorSession* UEditorEngine::GetPlaySession() {
    return &mPlaySession;
}

/**
 * @brief 공유 편집 컨텍스트의 비소유 주소를 반환합니다.
 */
FEditorContext* UEditorEngine::GetEditorContext() {
    return &mContext;
}

/**
 * @brief 에디터 뷰포트 클라이언트를 반환합니다.
 */
FEditorViewportClient* UEditorEngine::GetViewportClient() {
    return mViewport.GetClient();
}

/**
 * @brief 독립 뷰포트가 소유한 렌더 타깃을 반환합니다.
 */
FRenderTarget* UEditorEngine::GetViewportTarget() const {
    return mViewport.GetRenderTarget();
}

/**
 * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
 */
bool UEditorEngine::WantsExit() const {
    return mUI->WantsExit();
}

/**
 * @brief 엔진이 소유한 기본 독립 에디터 뷰포트의 비소유 주소를 반환합니다.
 */
FEditorViewport* UEditorEngine::GetEditorViewport() {
    return &mViewport;
}
