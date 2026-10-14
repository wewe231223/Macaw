#pragma once
#include <memory>
#include <optional>
#include "Core/Base/TStateFlags.h"
#include "RenderCore/FRenderData.h"
#include "Editor/View/FEditorViewport.h"

class UEditorEngine;
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
    FPIEViewportSession(UEditorEngine& Engine, FRenderer& Renderer, FViewportHostWindow* Host);
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
    void Render();
    void ReleaseRenderResources();
    FEditorViewport* GetViewport() const;
    bool IsInputCaptured() const;

private:
    void Synchronize();
    void SelectViewport();
    void ResetViewport();
    UCameraComponent* EnsureCamera(UWorld& World);
    bool BuildCamera(UWorld& World, FViewMatrices& Camera);
    void UpdateInput(UWorld& World);
    void MoveCamera(UWorld& World, float DeltaTime);
    static ImGuiKey GetImGuiKey(Uint8 Key);

private:
    UEditorEngine& mEngine;
    FRenderer& mRenderer;
    FViewportHostWindow* mHost{};
    std::unique_ptr<FEditorViewport> mWindowViewport{};
    FEditorViewport* mViewport{};
    std::optional<FViewportId> mTargetViewportId{};
    TStateFlags<EStateFlag> mState{};
};
