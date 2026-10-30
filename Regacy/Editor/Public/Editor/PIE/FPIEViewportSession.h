/**
 * @file FPIEViewportSession.h
 * @brief 전역 에디터 엔진을 사용하며 뷰포트별 입력과 표시 상태를 유지하는 PIE 세션을 정의합니다.
 */

#pragma once
#include <memory>
#include <optional>
#include "Core/Base/TStateFlags.h"
#include "RenderCore/FRenderData.h"
#include "Editor/View/FEditorViewport.h"

class FRenderer;
class FViewportHostWindow;

class FPIEViewportSession final {
private:
    enum class EStateFlag {
        InputCaptured,
        FocusWindow,
        InputResetPending,
        Count
    };

public:
    /**
     * @brief 현재 전역 에디터 엔진을 제어할 PIE 뷰포트 세션을 생성합니다.
     * @param Renderer PIE 씬을 렌더링할 렌더러입니다.
     * @param Host 선택적으로 PIE를 표시할 뷰포트 호스트입니다.
     * @pre 생성 시 GetEditorEngine이 편집 컨텍스트를 제공할 수 있어야 합니다.
     * @details 엔진 포인터를 보관하지 않으며 각 작업 시 현재 에디터 엔진을 조회합니다. 엔진 종료 후에는 실행 요청을 처리하지 않습니다.
     */
    FPIEViewportSession(FRenderer& Renderer, FViewportHostWindow* Host);
    ~FPIEViewportSession();
    FPIEViewportSession(const FPIEViewportSession&) = delete;

    FPIEViewportSession& operator=(const FPIEViewportSession&) = delete;

    FPIEViewportSession(FPIEViewportSession&&) = delete;

    FPIEViewportSession& operator=(FPIEViewportSession&&) = delete;

public:
    bool RequestStart();
    bool SetTargetViewport(std::optional<FViewportId> ViewportId);
    std::optional<FViewportId> GetTargetViewportId() const;
    void DrawControls();
    void BeginFrame();
    void DrawWindow();
    bool ProcessInput(FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, float DeltaTime);
    void Synchronize();
    void Render();
    void ReleaseRenderResources();
    FEditorViewport* GetViewport() const;
    bool IsInputCaptured() const;

private:
    void SelectViewport();
    void ResetViewport();
    UCameraComponent* EnsureCamera(UWorld& World);
    bool BuildCamera(UWorld& World, FViewMatrices& Camera);
    void UpdateInput(UWorld& World);
    void MoveCamera(UWorld& World, float DeltaTime);
    static ImGuiKey GetImGuiKey(Uint8 Key);

private:
    FRenderer& mRenderer;
    FViewportHostWindow* mHost{};
    std::unique_ptr<FEditorViewport> mWindowViewport{};
    FEditorViewport* mViewport{};
    std::optional<FViewportId> mTargetViewportId{};
    TStateFlags<EStateFlag> mState{};
};
