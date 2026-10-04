#pragma once
#include "Application/FApplication.h"

class FViewApplication final : public FApplication {
public:
    FViewApplication();
    ~FViewApplication() override;

private:
    void InitializeMode(FApplicationContext& Context, HWND WindowHandle) override;
    void RenderMode(FApplicationContext& Context, float DeltaTime) override;
};
