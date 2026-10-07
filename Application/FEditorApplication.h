#pragma once
#include "Application/FApplication.h"

class FEditorApplication final : public FApplication {
public:
    FEditorApplication();
    ~FEditorApplication() override;

private:
    void InitializeMode(FApplicationContext& Context, HWND WindowHandle) override;
    void ProcessInput(FApplicationContext& Context, float DeltaTime) override;
    void RenderMode(FApplicationContext& Context, float DeltaTime) override;

private:
    FOverlayRenderData mOverlayData{};
};
