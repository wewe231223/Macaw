/**
 * @file FViewportHostWindow.h
 * @brief 뷰포트별 레이아웃 상태와 전역 설정의 읽기 및 저장을 정의합니다.
 */

#pragma once

#include <array>
#include "Editor/Panel/FEditorWindow.h"
#include "Editor/View/FViewportPresetLayout.h"
#include "Editor/View/EditorViewport.h"
#include "Editor/View/FEditorViewport.h"
#include "Editor/Input/FKeyboardInput.h"
#include "Editor/Input/FMouseInput.h"
#include "Editor/World/FWorldEditorContext.h"
#include <d3d11.h>

class FStatPanel;

class FViewportHostWindow final : public FEditorWindow {
public:
    using FViewportId = ::FViewportId;
    static constexpr Uint32 MaximumViewportCount{FViewportPresetLayout::MaximumViewportCount};

public:
    FViewportHostWindow(ID3D11Device* Device, FWorldEditorContext& EditorContext);
    ~FViewportHostWindow() override;

public:
    void SetStatOverlay(std::unique_ptr<FStatPanel> Overlay);
    void PrepareFrame(ImGuiID InDockSpaceId);
    void ProcessInput(EditorViewport& Viewport, FKeyboardInput& KeyboardInput, FMouseInput& MouseInput, float DeltaTime);
    void ReleaseRenderResources() override;
    FEditorViewport* PrepareViewportForRender(FViewportId Id);
    Uint32 GetViewportCount() const;
    FEditorViewport* GetViewport(FViewportId Id) const;
    FEditorViewport* GetActiveViewport() const;

    /**
     * @brief 전역 설정의 레이아웃 프리셋과 분할 비율을 이 호스트의 레이아웃에 적용합니다.
     * @details 다른 호스트의 실행 중 레이아웃 상태는 변경하지 않습니다.
     */
    void ApplyLayoutSettings();

    /**
     * @brief 이 호스트의 현재 레이아웃을 전역 설정 원본에 기록합니다.
     * @note 파일 저장은 수행하지 않습니다.
     */
    void CaptureLayoutSettings() const;

private:
    void DrawContents() override;
    void PushWindowStyle() override;
    void SetViewportLayout(EViewportLayoutPreset InPreset);
    bool DrawSplitterHandle(SSplitter& Splitter);

private:
    std::unique_ptr<FStatPanel> mStatOverlay{};
    FViewportPresetLayout mLayout{};
    std::array<std::unique_ptr<FEditorViewport>, MaximumViewportCount> mViewports{};
    FViewportId mActiveViewportId{0};
    FWorldEditorContext& mEditorContext;
    ImGuiID mDockSpaceId{0};
    bool mBSplitterActive{false};

    bool mBHasPendingLayoutSettings{true};
};
