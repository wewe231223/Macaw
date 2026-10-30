/**
 * @file FObjViewerApplication.cpp
 * @brief 독립 OBJ 뷰어의 실행과 파일 열기입니다.
 * @ingroup App
 */
#include "FObjViewerApplication.h"
#include "Core/Stats.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Editor/FMeshViewer.h"

#include <commdlg.h>
#include <shellapi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

/**
 * @brief ImGui의 Win32 메시지 처리 함수입니다.
 * @param Window 창 핸들입니다.
 * @param Message 메시지 종류입니다.
 * @param WParam 메시지 인수입니다.
 * @param LParam 메시지 인수입니다.
 * @return 메시지 처리 결과입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

namespace App {

    App::FObjViewerApplication::FObjViewerApplication() = default;

    App::FObjViewerApplication::~FObjViewerApplication() {
        Shutdown();
    }

    int App::FObjViewerApplication::Run() {
        if (!mPlatform.Create(L"Macaw OBJ Viewer", 1280, 900)) {
            return 1;
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};

        if (Length == 0 || Length >= Executable.size()) {
            return 1;
        }

        const auto Directory{std::filesystem::path{Executable.data()}.parent_path()};
        const auto Content{Directory / "Viewer/Content"};
        std::error_code Code{};
        std::filesystem::create_directories(Content, Code);

        if (!Code) {
            std::filesystem::copy(Directory / "Templates/Project/Content", Content, std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, Code);
        }

        if (Code) {
            MessageBoxW(mPlatform.GetWindowHandle(), L"Cannot prepare viewer content. Build the application with its project templates.", L"OBJ Viewer", MB_OK | MB_ICONERROR);
            return 1;
        }

        mAssets = std::make_unique<Engine::FAssetManager>(Content);

        Core::FString MigrationError{};

        if (!mAssets->MigrateMaterials(MigrationError)) {
            MessageBoxW(mPlatform.GetWindowHandle(), Core::FilePath::FromUtf8(MigrationError).c_str(), L"Material Migration", MB_OK | MB_ICONERROR);
            return 1;
        }

        mRenderer.SetProjectPaths(Content, Directory / "Viewer/Saved/ShaderCache");

        if (!mRenderer.Initialize(mPlatform.GetWindowHandle(), mPlatform.GetWidth(), mPlatform.GetHeight())) {
            return 1;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        mContextCreated = true;
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::GetIO().LogFilename = nullptr;
        ImGui::StyleColorsDark();
        mUIScale.Initialize(16.0f);
        mUIScale.Update(mPlatform);
        mWin32Ready = ImGui_ImplWin32_Init(mPlatform.GetWindowHandle());
        mDxReady = mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(mRenderer.GetNativeDevice()), static_cast<ID3D11DeviceContext*>(mRenderer.GetNativeContext()));

        if (!mDxReady) {
            return 1;
        }

        mPlatform.SetMessageHandler([](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
        });

        mViewer = std::make_unique<Editor::FMeshViewer>(&mRenderer, mAssets.get());

        int ArgumentCount{};
        wchar_t** Arguments{CommandLineToArgvW(GetCommandLineW(), &ArgumentCount)};

        if (Arguments != nullptr) {
            if (ArgumentCount > 1) {
                OpenSource(std::filesystem::path{Arguments[1]});
            }

            LocalFree(Arguments);
        }

        while (mPlatform.PumpMessages()) {
            if (mPlatform.IsMinimized()) {
                MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
                continue;
            }

            const Core::FScopedStatFrame StatFrame{};

            mRenderer.BeginFrame(mPlatform.GetWidth(), mPlatform.GetHeight());
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            mUIScale.Update(mPlatform);
            ImGui::NewFrame();

            if (ImGui::BeginMainMenuBar()) {
                mUIScale.DrawMenu();

                if (ImGui::MenuItem("Open OBJ / Mesh...")) {
                    const auto Source{ChooseSource()};

                    if (!Source.empty()) {
                        OpenSource(Source);
                    }
                }

                ImGui::EndMainMenuBar();
            }

            if (!mViewer->IsOpen()) {
                ImGui::Begin("Open Mesh");
                ImGui::TextUnformatted("Choose Open OBJ / Mesh to load an OBJ, FBX or glTF file.");
                ImGui::End();
            }

            mViewer->DrawUI(true);

            if (!mError.empty()) {
                ImGui::OpenPopup("Cannot Open Mesh");
            }

            if (ImGui::BeginPopupModal("Cannot Open Mesh", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                ImGui::TextWrapped("%s", mError.c_str());

                if (ImGui::Button("Close")) {
                    mError.clear();
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            ImGui::Render();
            mRenderer.BeginUiRender();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            mRenderer.Present();
        }

        Shutdown();

        return 0;
    }

    bool App::FObjViewerApplication::OpenSource(const std::filesystem::path& Source) {
        std::filesystem::path Target{};

        if (!mAssets->ImportAsset(Source, "Meshes", Target, mError) || !mViewer->Open(Target, mError)) {
            return false;
        }

        mError.clear();

        return true;
    }

    std::filesystem::path App::FObjViewerApplication::ChooseSource() {
        std::array<wchar_t, 32768> Buffer{};
        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform.GetWindowHandle();
        Dialog.lpstrTitle = L"Open OBJ / Mesh";
        Dialog.lpstrFilter = L"Mesh files\0*.obj;*.fbx;*.gltf;*.glb\0All files\0*.*\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

        return GetOpenFileNameW(&Dialog) ? std::filesystem::path{Buffer.data()} : std::filesystem::path{};
    }

    void App::FObjViewerApplication::Shutdown() {
        mPlatform.SetMessageHandler({});
        mViewer.reset();

        if (mDxReady) {
            ImGui_ImplDX11_Shutdown();
            mDxReady = false;
        }

        if (mWin32Ready) {
            ImGui_ImplWin32_Shutdown();
            mWin32Ready = false;
        }

        if (mContextCreated) {
            ImGui::DestroyContext();
            mContextCreated = false;
        }

        mAssets.reset();
        mRenderer.Shutdown();
        mPlatform.Shutdown();
    }

}
