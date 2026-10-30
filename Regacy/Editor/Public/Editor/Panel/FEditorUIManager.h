/**
 * @file FEditorUIManager.h
 * @brief 전역 엔진, 설정과 에셋 서비스를 사용하는 에디터 UI 초기화를 정의합니다.
 */

#pragma once
#include "Core/CoreMinimal.h"
#include "ImGui/imgui.h"
#include "Editor/Message/FEditorInfo.h"
#include "Editor/Panel/FViewportHostWindow.h"
#include <filesystem>
#include "World/UWorld.h"
#include "Render/Renderer.h"
#include "Editor/World/FWorldEditorContext.h"
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Editor/Panel/FAssetBrowserPanel.h"
#include "Editor/Panel/FMaterialEditorPanel.h"
#include "Editor/Panel/FViewerPanel.h"
#include "Editor/Panel/FStatPanel.h"
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/Panel/IEditorPanel.h"
#include "Editor/PIE/FPIEViewportSession.h"
#include "Editor/View/EditorViewport.h"

/** @brief 전역 엔진 서비스로 패널을 구성하고 패널 및 PIE 뷰포트의 UI 수명을 관리합니다. */
class FEditorUIManager {
public:
    /** @brief 패널이 없는 UI 관리자를 생성합니다. */
    FEditorUIManager();

    /** @brief 소유한 패널과 PIE 뷰포트 세션을 해제합니다. */
    ~FEditorUIManager();

public:
    /**
     * @brief 전역 에디터 엔진, 설정과 에셋 레지스트리를 사용해 편집기 패널을 생성합니다.
     * @param Renderer 패널과 PIE 뷰포트가 사용할 렌더러입니다.
     * @param WindowHandle 입력과 파일 선택에 사용할 기본 창 핸들입니다.
     * @param ThumbnailRenderer 선택적으로 사용할 썸네일 렌더러입니다. nullptr이면 썸네일 기능을 사용하지 않습니다.
     * @pre GetEditorEngine과 FAssetRegistry::Get으로 접근할 엔진과 에셋 서비스가 준비되어 있어야 합니다.
     */
    void Initialize(FRenderer& Renderer, HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer);

    /**
     * @brief 전역 에디터 엔진의 컨텍스트와 에셋 서비스를 사용해 뷰어 패널을 생성합니다.
     * @param WindowHandle 뷰어 입력을 받을 창 핸들입니다.
     * @param ThumbnailRenderer 선택적으로 사용할 썸네일 렌더러입니다.
     * @pre 전역 에디터 엔진과 에셋 서비스가 준비되어 있어야 합니다.
     */
    void InitializeViewer(HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer);

    /** @brief 도킹과 패널 표시 및 PIE 창을 한 프레임 갱신합니다. */
    void Tick();

    /**
     * @brief 보이는 패널의 오프스크린 렌더링을 실행합니다. 각 패널은 에셋을 전역 서비스에서 조회합니다.
     * @param Renderer 패널 프리뷰를 그릴 렌더러입니다.
     */
    void RenderOffscreen(FRenderer& Renderer);

    /** @brief 소유 패널과 PIE 뷰포트의 렌더 자원을 해제합니다. */
    void ReleaseRenderResources();

    /** @brief 현재 도킹 영역 식별자를 반환합니다. */
    ImGuiID GetDockSpaceId() const;

    /** @brief 뷰포트 호스트 창을 반환하며 생성하지 않았으면 nullptr입니다. */
    FViewportHostWindow* GetViewportHostWindow() const;

    /** @brief PIE 뷰포트 세션을 반환하며 생성하지 않았으면 nullptr입니다. */
    FPIEViewportSession* GetPIEViewportSession() const;

    /** @brief PIE 세션이 있으면 실행 제어 UI를 표시합니다. */
    void DrawPIEControls();

    /** @brief 외부 파일 드롭을 현재 실행 모드의 패널로 전달합니다. @param FilePath 드롭한 파일 경로입니다. @param ScreenPosition 화면 좌표입니다. @return 패널이 드롭을 처리했으면 true입니다. */
    bool HandleExternalFileDrop(const std::filesystem::path& FilePath, const ImVec2& ScreenPosition);
    void DrawMainMenu();
    void ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, float DeltaTime);
    const FRenderScene* UpdateRenderData(FRenderer& Renderer);
    void RenderViews(FRenderer& Renderer, const FRenderScene* RenderScene);

private:
    /** @brief 패널의 소유권을 관리자에 추가합니다. @param Window 추가할 패널입니다. */
    void AddWindow(std::unique_ptr<FEditorWindow> Window);

    /**
     * @brief 전역 설정에 따라 뷰어 패널을 생성합니다.
     * @param EditorContext 프리뷰 요청을 주고받는 월드별 편집 컨텍스트입니다.
     * @param WindowHandle 입력 창입니다.
     * @param ThumbnailRenderer 선택적인 썸네일 생성기입니다.
     */
    void AddViewerWindow(FWorldEditorContext& EditorContext, HWND WindowHandle, FAssetThumbnailRenderer* ThumbnailRenderer);

    /** @brief 에디터 뷰포트 호스트를 생성합니다. @param Device 그래픽 장치입니다. @param EditorContext 편집 컨텍스트입니다. */
    void AddViewportHostWindow(ID3D11Device* Device, FWorldEditorContext& EditorContext);

private:
    std::unique_ptr<IEditorPanel> mMenuPanel{};
    EditorViewport mEditorView{};
    FOverlayRenderData mOverlayData{};
    bool mViewerOnly{};

    /** @brief 생성 순서대로 소유하는 UI 패널 목록입니다. */
    std::vector<std::unique_ptr<IEditorPanel>> mElements{};

    /** @brief 소유 목록에 등록된 뷰포트 호스트의 비소유 참조입니다. */
    FViewportHostWindow* mViewportHostWindow{};

    /** @brief PIE 뷰포트 세션의 소유 포인터입니다. */
    std::unique_ptr<FPIEViewportSession> mPIEViewportSession{};

    /** @brief 소유 목록에 등록된 뷰어 패널의 비소유 참조입니다. */
    FViewerPanel* mViewerWindow{};

    /** @brief 프리뷰 열기 요청을 읽을 편집 컨텍스트입니다. */
    FWorldEditorContext* mPreviewContext{};

    /** @brief 현재 프레임의 도킹 영역 식별자입니다. */
    ImGuiID mDockSpaceId{};

    /** @brief 에셋 브라우저 패널의 비소유 참조입니다. */
    FAssetBrowserPanel* mAssetBrowserPanel{};

    /** @brief 머티리얼 편집 패널의 비소유 참조입니다. */
    FMaterialEditorPanel* mMaterialEditorPanel{};

    /** @brief 다음 표시 시 머티리얼 편집기로 포커스를 이동할지 나타냅니다. */
    bool mFocusMaterialEditor{};

    /** @brief 통계 패널과 콘솔이 공유하는 표시 설정입니다. */
    FStatDisplayFlags mStatDisplayFlags{};
};
