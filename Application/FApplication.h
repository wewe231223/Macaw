#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <vector>
#include <shellapi.h>

#include "IApplication.h"
#include "FApplicationContext.h"

class FLoadingProgress;

class FApplication : public IApplication {
private:
    struct FPendingExternalFileDrop {
        std::filesystem::path mFilePath{};
        POINT mScreenPosition{};
    };

public:
    FApplication();
    ~FApplication() override;

public:
    int Run(HINSTANCE Instance, int ShowCommand) final;

private:
    virtual void InitializeMode(FApplicationContext& Context, HWND WindowHandle) = 0;
    virtual void TickMode(FApplicationContext& Context, float DeltaTime) = 0;

    bool InitializeApplication(FLoadingProgress& Progress, HWND WindowHandle);
    void RegisterObjectTypes();
    int RunMessageLoop(HACCEL AcceleratorTable);
    void RenderFrame();
    void SaveState();
    void Shutdown();

    ATOM RegisterWindowClass(HINSTANCE Instance);
    bool CreateApplicationWindow(HINSTANCE Instance, int ShowCommand);
    void RestoreGameWindow();
    void DrawCaptionButton(const char* Identifier, int Index, UINT Command);
    void DrawTitleBar();

    void QueueExternalFileDrops(HWND WindowHandle, HDROP DropHandle);
    void EnableExternalDropsForImGuiViewports();
    LRESULT ProcessWindowMessage(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);
    static LRESULT CALLBACK WindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);
    static LRESULT CALLBACK ExternalDropWindowProcedure(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam);

private:
    static constexpr UINT mLoadingWindowWidth{960};
    static constexpr UINT mLoadingWindowHeight{540};
    FApplicationContext mContext{};
    HWND mWindowHandle{};
    int mMenuHitRight{900};
    bool mCustomFrameEnabled{};
    bool mRenderingFrame{};
    bool mInMoveLoop{};
    bool mFrameEnabled{};
    bool mImGuiInitialized{};
    std::atomic<bool> mAcceptGameInput{};
    std::chrono::steady_clock::time_point mLastTickTime{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mEditorLogo{};
    std::vector<FPendingExternalFileDrop> mPendingExternalFileDrops{};
};
