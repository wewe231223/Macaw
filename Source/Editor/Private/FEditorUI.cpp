/**
 * @file FEditorUI.cpp
 * @brief 편집 상태를 소유하지 않는 ImGui 창, 패널과 뷰포트 호스트를 구현합니다.
 * @ingroup Editor
 */

#include "FEditorUI.h"
#include "Core/Stats.h"
#include "Core/FilePath.h"
#include "EditorStyle.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "Platform/Window.h"
#include "Platform/WindowResources.h"
#include "RenderCore/IRendererModule.h"
#include "Engine/FProject.h"
#include "Asset/FAssetFile.h"
#include "Asset/UTexture2D.h"
#include "Asset/UMaterial.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Import/IMeshImporter.h"
#include "Engine/Component/ULightComponentBase.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/AActor.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Renderer/Renderer.h"
#include "Engine/SceneSerializer.h"
#include "Engine/FWorldContext.h"
#include "Editor/FAssetDragDrop.h"
#include "Editor/FMeshViewer.h"
#include "FBlendSpaceEditor.h"
#include "FAnimGraphEditor.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FGameExporter.h"
#include "Editor/FProjectDialog.h"

#include <d3d11.h>
#include <commdlg.h>
#include <shellapi.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <regex>
#include <span>

/**
 * @brief ImGui Win32 백엔드의 외부 메시지 처리 진입점입니다.
 * @param[in] Window 메시지를 받은 Win32 창입니다.
 * @param[in] Message Win32 메시지 번호입니다.
 * @param[in] WParam Windows 메시지의 WPARAM 인수입니다.
 * @param[in] LParam Windows 메시지의 LPARAM 인수입니다.
 * @return ImGui 백엔드의 메시지 처리 결과입니다.
 */
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        struct FContentIcon {
            std::string_view mKind{};
            const char* mTooltip{};
            WORD mResourceId{};
            Editor::EditorStyle::EIcon mFallback{};
            ImU32 mTint{IM_COL32_WHITE};
        };

        constexpr FContentIcon FileIcon{"File", "파일", MacawContentFileIconResource, Editor::EditorStyle::EIcon::Component};
        constexpr FContentIcon ContentIcons[]{
            FileIcon,
            {"Folder", "폴더", MacawContentFolderIconResource, Editor::EditorStyle::EIcon::Component, IM_COL32(255, 208, 111, 255)},
            {"C# Script", "C# 스크립트", MacawContentScriptIconResource, Editor::EditorStyle::EIcon::Tools, IM_COL32(216, 179, 255, 255)},
            {"Mesh", "메시", MacawContentMeshIconResource, Editor::EditorStyle::EIcon::Local},
            {"Skeletal Mesh", "스켈레탈 메시", MacawContentSkeletalMeshIconResource, Editor::EditorStyle::EIcon::Skeleton},
            {"Skeleton", "스켈레톤", MacawContentSkeletonIconResource, Editor::EditorStyle::EIcon::Skeleton},
            {"Material", "머티리얼", MacawContentMaterialIconResource, Editor::EditorStyle::EIcon::Grid},
            {"Texture", "텍스처", MacawContentTextureIconResource, Editor::EditorStyle::EIcon::Sprite},
            {"Sequence", "시퀀스", MacawContentSequenceIconResource, Editor::EditorStyle::EIcon::Sequence},
            {"Animation Sequence", "애니메이션 시퀀스", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence},
            {"Blend Space", "블렌드 스페이스", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence, IM_COL32(120, 214, 236, 255)},
            {"Anim Graph", "애니메이션 그래프", MacawContentAnimationSequenceIconResource, Editor::EditorStyle::EIcon::Sequence, IM_COL32(150, 165, 255, 255)},
            {"Scene", "씬", MacawContentSceneIconResource, Editor::EditorStyle::EIcon::World}
        };

        const FContentIcon& GetContentIcon(std::string_view Kind) {
            for (const FContentIcon& Icon : ContentIcons) {
                if (Icon.mKind == Kind) {
                    return Icon;
                }
            }

            return FileIcon;
        }

        void DrawContentImage(float Size, void* Thumbnail, ID3D11ShaderResourceView* Resource, const FContentIcon& Icon) {
            if (Thumbnail != nullptr) {
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {Size, Size});

                return;
            }

            const float IconSize{Size * 0.8f};
            const float Inset{(Size - IconSize) * 0.5f};
            const ImVec2 Cursor{ImGui::GetCursorScreenPos()};
            const ImVec2 Origin{Cursor.x + Inset, Cursor.y + Inset};
            ImDrawList* DrawList{ImGui::GetWindowDrawList()};

            if (Resource != nullptr) {
                DrawList->AddImage(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Resource))}, Origin, {Origin.x + IconSize, Origin.y + IconSize}, {0.0f, 0.0f}, {1.0f, 1.0f}, Icon.mTint);
            } else if (Icon.mKind == "Folder") {
                DrawList->AddRectFilled({Origin.x, Origin.y + IconSize * 0.22f}, {Origin.x + IconSize * 0.45f, Origin.y + IconSize * 0.45f}, Icon.mTint, IconSize * 0.04f);
                DrawList->AddRectFilled({Origin.x, Origin.y + IconSize * 0.33f}, {Origin.x + IconSize, Origin.y + IconSize * 0.83f}, Icon.mTint, IconSize * 0.05f);
            } else {
                Editor::EditorStyle::DrawIcon(Icon.mFallback, Origin, IconSize, Icon.mTint);
            }

            ImGui::Dummy({Size, Size});
        }
    }

    /**
     * @brief 편집 컨텍스트를 패널에 연결하고 독립 뷰포트의 배치 연결을 구성합니다.
     */
    Editor::FEditorUI::FEditorUI(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession)
		: mContext{Context},
		  mViewport{Viewport},
		  mPlaySession{PlaySession},
		  mScriptBuilds{Context},
		  mViewportLayout{Context, Viewport, PlaySession},
		  mPlaceActorsPanel{Context},
		  mOutlinerPanel{Context},
		  mDetailsPanel{Context},
		  mConsolePanel{Context},
		  mSequencerPanel{Context},
		  mAnimGraphPanel{Context} {
    }

    /**
     * @brief UI 백엔드와 표시 리소스를 정리합니다.
     */
    Editor::FEditorUI::~FEditorUI() {
        Shutdown();
    }

    /**
     * @brief 독립 뷰포트를 배치할 ImGui 백엔드를 준비합니다.
     */
    bool Editor::FEditorUI::Initialize(RenderCore::IRendererModule* Renderer, Platform::FWindowsApplication* Platform) {
        if (Renderer == nullptr || Platform == nullptr || mContextCreated || mContext->GetProject() == nullptr) {
            return false;
        }

        mPlatform = Platform;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        mContextCreated = true;

        ImGuiIO& Input{ImGui::GetIO()};
        Input.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;

        const std::filesystem::path Layout{mContext->GetProject()->GetRootPath() / "Saved/EditorLayout.ini"};
        std::error_code LayoutError{};

        std::filesystem::create_directories(Layout.parent_path(), LayoutError);

        mIniPath = Core::FilePath::ToUtf8(Layout);
        Input.IniFilename = mIniPath.c_str();
        Input.LogFilename = nullptr;

        if (std::filesystem::exists("C:/Windows/Fonts/malgun.ttf")) {
            Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/malgun.ttf", 16.0f);
        } else if (std::filesystem::exists("C:/Windows/Fonts/segoeui.ttf")) {
            Input.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f);
        } else {
            Input.Fonts->AddFontDefault();
        }

        ImGui::StyleColorsDark();

        ImGuiStyle& Style{ImGui::GetStyle()};
        Style.WindowRounding = 3.0f;
        Style.FrameRounding = 3.0f;
        Style.GrabRounding = 3.0f;

        mUIScale.Initialize(16.0f);
        mUIScale.Update(*Platform);

        mWin32Ready = ImGui_ImplWin32_Init(Platform->GetWindowHandle());
        mDxReady = mWin32Ready && ImGui_ImplDX11_Init(static_cast<ID3D11Device*>(Renderer->GetNativeDevice()), static_cast<ID3D11DeviceContext*>(Renderer->GetNativeContext()));

        if (!mDxReady || !mViewportLayout.Initialize(Renderer)) {
            Shutdown();

            return false;
        }

        Platform->SetMessageHandler([this](HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
            if (Message == WM_DROPFILES) {
                QueueFileDrop(reinterpret_cast<void*>(WParam));
                return true;
            }

            if (Message == WM_SETFOCUS) {
                mScriptBuilds.NotifyFocusGained();
            }

            return ImGui_ImplWin32_WndProcHandler(Window, Message, WParam, LParam) != 0;
        });

        mInitialized = true;

        ImGui::LoadIniSettingsFromDisk(mIniPath.c_str());
        InitializeSettings();
        InstallEditorIcons();
        LoadContentIcons(Renderer);

        if (mContext->GetWorld() != nullptr && mContext->GetWorld()->GetPersistentLevel() != nullptr) {
            for (const auto& Actor : mContext->GetWorld()->GetPersistentLevel()->GetActors()) {
                for (const auto& Component : Actor->GetComponents()) {
                    if (auto* Light{CoreUObject::Cast<Engine::ULightComponentBase>(Component.get())}) {
                        Light->SynchronizeLight();
                    }
                }
            }
        }

        mAssetCatalog = std::make_unique<Editor::FEditorAssetCatalog>(mContext->GetProject()->GetAssets());
        mThumbnails = std::make_unique<Editor::FAssetThumbnailCache>(mContext->GetProject()->GetAssets(), Renderer);
        mDetailsPanel.SetAssetServices(mAssetCatalog.get(), mThumbnails.get());
        mDetailsPanel.SetAssetNavigation([this]() {
            return mSelectedContentPath;
        }, [this](const std::filesystem::path& Path) {
            mContentFilter.reset();
            mContentSearch.fill('\0');
            NavigateContent(Path.parent_path());
            mSelectedContentPath = Path;
            ImGui::SetWindowFocus("Content");
        });
        mDetailsPanel.SetScriptBuildService(&mScriptBuilds);
        mMeshViewer = std::make_unique<Editor::FMeshViewer>(Renderer, mContext->GetProject()->GetAssets());
        mBlendSpaceEditor = std::make_unique<Editor::FBlendSpaceEditor>(mContext->GetProject()->GetAssets(), mAssetCatalog.get(), mThumbnails.get());
        mAnimGraphEditor = std::make_unique<Editor::FAnimGraphEditor>(mContext->GetProject()->GetAssets(), mAssetCatalog.get(), mThumbnails.get());
        mDetailsPanel.SetAnimGraphOpener([this]() {
            mAnimGraphPanel.Open();
        });
        mExit = false;
        mRefreshContent = true;

        RefreshContent();
        DragAcceptFiles(Platform->GetWindowHandle(), TRUE);

        return true;
    }

    /**
     * @brief 프로젝트의 Editor/Icon 폴더에 없는 언리얼 아이콘과 출처 파일을 배포 템플릿에서 복사합니다.
     */
    void Editor::FEditorUI::InstallEditorIcons() {
        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        const std::filesystem::path Bundled{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content/Editor/Icon" : std::filesystem::path{}};
        Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};

        for (const char* Name : {"SpotLight_64x.png", "PointLight_64x.png", "DirectionalLight_64x.png", "SkyLight_64x.png", "Pawn_64x.png", "ExponentialHeightFog_64x.png", "Sources.txt"}) {
            Core::FString Error{};
            const std::filesystem::path Target{Assets->ResolvePath(std::filesystem::path{"Editor/Icon"} / Name, Error)};
            std::error_code Code{};

            if (Target.empty()) {
                mContext->AddLog(Error);

                continue;
            }

            if (std::filesystem::is_regular_file(Target, Code)) {
                continue;
            }

            std::filesystem::path Source{Bundled / Name};

            if (Bundled.empty() || !std::filesystem::is_regular_file(Source, Code)) {
                Source = std::filesystem::path{"Templates/Project/Content/Editor/Icon"} / Name;
            }

            if (!std::filesystem::is_regular_file(Source, Code)) {
                mContext->AddLog("Cannot find bundled editor icon: " + Core::FString{Name});

                continue;
            }

            std::filesystem::create_directories(Target.parent_path(), Code);

            if (!Code) {
                std::filesystem::copy_file(Source, Target, std::filesystem::copy_options::skip_existing, Code);
            }

            if (Code) {
                mContext->AddLog("Cannot install editor icon '" + Core::FString{Name} + "': " + Code.message());
            }
        }
    }

    void Editor::FEditorUI::LoadContentIcons(RenderCore::IRendererModule* Renderer) {
        auto* Device{static_cast<ID3D11Device*>(Renderer->GetNativeDevice())};

        if (Device == nullptr) {
            return;
        }

        const HMODULE Module{GetModuleHandleW(nullptr)};
        const std::filesystem::path ImageName{L"ContentIcon.png"};

        for (const FContentIcon& Icon : ContentIcons) {
            const HRSRC Resource{FindResourceW(Module, MAKEINTRESOURCEW(Icon.mResourceId), RT_RCDATA)};
            const HGLOBAL Data{Resource != nullptr ? LoadResource(Module, Resource) : nullptr};
            const auto* Bytes{Data != nullptr ? static_cast<const Core::Uint8*>(LockResource(Data)) : nullptr};

            if (Bytes == nullptr) {
                continue;
            }

            const std::span<const Core::Uint8> Image{Bytes, SizeofResource(Module, Resource)};
            Asset::UTexture2D Texture{};

            if (Texture.LoadFromMemory(Image, ImageName, false) && Texture.CreateResources(Device)) {
                mContentIcons.try_emplace(Icon.mKind, Texture.GetShaderResourceView());
            }
        }
    }

    /**
     * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
     */
    void Editor::FEditorUI::Shutdown() {
        if (mInitialized) {
            SaveSettings();
        }

        if (mPlatform != nullptr) {
            const HWND Window{mPlatform->GetWindowHandle()};

            if (Window != nullptr) {
                DragAcceptFiles(Window, FALSE);

                MSG Message{};

                while (PeekMessageW(&Message, Window, WM_DROPFILES, WM_DROPFILES, PM_REMOVE)) {
                    DragFinish(reinterpret_cast<HDROP>(Message.wParam));
                }
            }
        }

        mDroppedFiles.clear();
        mDropDirectories.clear();
        mContentDropDirectories.clear();
        mContentDropBounds.clear();
        mPlaceActorsPanel.ClearIcons();
        mContentIcons.clear();
        mMeshViewer.reset();
        mBlendSpaceEditor.reset();
        mAnimGraphEditor.reset();
        mDetailsPanel.SetAnimGraphOpener({});
        mDetailsPanel.SetAssetServices(nullptr, nullptr);
        mDetailsPanel.SetAssetNavigation({}, {});
        mThumbnails.reset();
        mAssetCatalog.reset();
        mContentSearch.fill('\0');
        mContentFilter.reset();

        if (mContextCreated && ImGui::GetIO().IniFilename != nullptr) {
            if (ImGui::GetCurrentContext()->SettingsLoaded) {
                ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
            }

            ImGui::GetIO().IniFilename = nullptr;
        }

        mViewportLayout.Shutdown();

        if (mPlatform != nullptr) {
            mPlatform->SetMessageHandler({});
        }

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

        mInitialized = false;
        mAssetPaths.clear();
        mContentFolders.clear();
        mAssetTypes.clear();
        mContentDirectory.clear();
        mSelectedContentPath.clear();
        mContentError.clear();
        mRevealContentDirectory = true;
        mPreviewTexture = nullptr;
        mPreviewTexturePath.clear();
        mFocusTexturePreview = false;
        mEditingMaterial = nullptr;
        mMaterialPath.clear();
        mMaterialError.clear();
        mMaterialDirty = false;
        mPlatform = nullptr;
        mSettingsOpen = false;
        mAnimSequenceOpen = false;
        mAnimSequencePath.clear();
        mFocusSettings = false;
        mSettingsDirty = false;

        mExit = true;
    }

    /**
     * @brief 프레임 입력과 패널 UI를 구성합니다.
     */
    bool Editor::FEditorUI::BeginFrame(float DeltaTime) {
        static Core::FStat UiTime{"unit", "Editor UI CPU", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{UiTime};

        if (!mInitialized) {
            return false;
        }

        mContext->GetRenderer()->BeginFrame(mPlatform->GetWidth(), mPlatform->GetHeight());

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();

        if (mAppearanceDirty) {
            mUIScale.SetBaseStyle(mThemes.GetStyle(mThemeIndex), mFontSize);
            mAppearanceDirty = false;
        }

        mUIScale.Update(*mPlatform);
        ImGui::NewFrame();
        mDetailsPanel.FinishInactiveSceneEdit();
        mThumbnails->BeginFrame();
        ImportDroppedFiles();
        mScriptBuilds.Update(!mPlaySession->IsActive());
        UpdatePendingPlay();

        if (mRefreshContent) {
            RefreshContent();
        }

        if (mEditingMaterial != nullptr) {
            mThumbnails->GetMaterialPreview(mEditingMaterial);
        }

        DrawMenu();
        DrawEditorToolbar();
        BuildLayout();

        Editor::FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};

        mOutlinerPanel.SetEditorContext(Inspection);
        mDetailsPanel.SetEditorContext(Inspection);
        mPlaceActorsPanel.Draw(DeltaTime);
        mOutlinerPanel.Draw(DeltaTime);
        mDetailsPanel.Draw(DeltaTime);
        mDetailsPanel.FinishInactiveSceneEdit();
        mViewportLayout.DrawUI(mPlatform->GetInput(), DeltaTime);
        mConsolePanel.Draw(DeltaTime);

        // 패널을 닫으면 그리기가 멈추므로 프리뷰 값이 월드에 남지 않도록 여기서 복원합니다.
        if (!mSequencerPanel.IsVisible()) {
            mSequencerPanel.RestorePreview();
        }

        mSequencerPanel.Draw(DeltaTime);
        mAnimGraphPanel.SetEditorContext(Inspection);
        mAnimGraphPanel.Draw(DeltaTime);

        Editor::FStatGraphs& StatGraphs{mContext->GetStatGraphs()};

        GetActiveViewport()->GetStatistics()->DrawPanel(mShowStatistics, StatGraphs);
        StatGraphs.Draw();
        DrawContent();
        DrawTexturePreview();
        DrawMaterialEditor();
        DrawSettings();
        DrawImportOptions();

        if (mSettingsDirty) {
            mSettingsSaveDelay += DeltaTime;

            if (mSettingsSaveDelay >= 2.0f) {
                SaveSettings();

                mSettingsSaveDelay = 0.0f;
            }
        }

        if (mMeshViewer != nullptr) {
            mMeshViewer->DrawUI(mPlatform->GetInput());
        }

        if (mBlendSpaceEditor != nullptr) {
            mBlendSpaceEditor->Draw();
        }

        if (mAnimGraphEditor != nullptr) {
            mAnimGraphEditor->Draw();
        }

        DrawAnimSequenceSettings();

        mPlaySession->ProcessInput(DeltaTime, !ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup));

        return true;
    }

    /**
     * @brief 동기화한 장면과 통계를 그린 뒤 ImGui 드로 데이터를 제출합니다.
     */
    void Editor::FEditorUI::EndFrame(float DeltaTime) {
        if (!mInitialized) {
            return;
        }

        mViewportLayout.Draw(DeltaTime);
        DrawShadowAtlas();

        static Core::FStat UiSubmitTime{"unit", "ImGui submission CPU", Core::EStatUnit::Milliseconds};
        const Core::FScopedStatTimer StatTimer{UiSubmitTime};

        ImGui::Render();
        mContext->GetRenderer()->BeginUiRender();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    /**
     * @brief 그림자 아틀라스 미리보기와 타일별 광원 정보를 그립니다.
     */
    void Editor::FEditorUI::DrawShadowAtlas() {
        if (!mContext->IsShadowAtlasOpen()) {
            return;
        }

        bool Open{true};

        ImGui::SetNextWindowSize({760.0f, 840.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Shadow Atlas", &Open)) {
            const RenderCore::FShadowAtlasView Atlas{mContext->GetRenderer()->GetShadowAtlasView()};
            const RenderCore::FRenderStats Stats{mContext->GetRenderer()->GetStats()};

            ImGui::Text("%u x %u | D24S8 | %u maps", Atlas.mSize, Atlas.mSize, Stats.mShadowMaps);
            ImGui::Text("Resources: %.1f MiB | Allocated tiles: %.1f MiB", Stats.mShadowAtlasBytes / (1024.0 * 1024.0), Stats.mShadowAllocatedBytes / (1024.0 * 1024.0));
            ImGui::Text("Directional: %u | Point: %u | Spot: %u", Stats.mDirectionalLights, Stats.mPointLights, Stats.mSpotLights);
            ImGui::Text("Reduced: %u | Unallocated lights: %u | Skipped lights: %u | Cached tiles: %u", Stats.mShadowReducedLights, Stats.mShadowDroppedLights, Stats.mShadowSkippedLights, Stats.mShadowCachedTiles);
            ImGui::SliderFloat("Zoom", &mShadowAtlasZoom, 1.0f, 8.0f, "%.1fx");

            if (Atlas.mTexture != nullptr) {
                const float Size{std::max(256.0f, ImGui::GetContentRegionAvail().x - 20.0f) * mShadowAtlasZoom};

                if (ImGui::BeginChild("AtlasImage", {}, 0, ImGuiWindowFlags_HorizontalScrollbar)) {
                    const ImVec2 Origin{ImGui::GetCursorScreenPos()};

                    ImGui::Image(reinterpret_cast<ImTextureID>(Atlas.mTexture), {Size, Size});

                    ImDrawList* Draw{ImGui::GetWindowDrawList()};
                    Draw->AddRect(Origin, {Origin.x + Size, Origin.y + Size}, IM_COL32(255, 230, 0, 255));

                    for (const RenderCore::FShadowAtlasTile& Tile : Atlas.mTiles) {
                        const ImVec2 Minimum{Origin.x + Tile.mRectangle.x * Size, Origin.y + Tile.mRectangle.y * Size};
                        const ImVec2 Maximum{Minimum.x + Tile.mRectangle.z * Size, Minimum.y + Tile.mRectangle.w * Size};

                        Draw->AddRect(Minimum, Maximum, IM_COL32(255, 230, 0, 255), 0.0f, 0, 1.5f);

                        if (ImGui::IsMouseHoveringRect(Minimum, Maximum)) {
                            ImGui::SetTooltip("맵 %u | %u x %u", Tile.mFace, Tile.mResolution, Tile.mResolution);
                        }
                    }
                }

                ImGui::EndChild();
            } else {
                ImGui::TextWrapped("No active shadow maps. Enable Shadows in a lit viewport and enable CastShadows on a light.");
            }
        }

        ImGui::End();
        mContext->SetShadowAtlasOpen(Open);
    }

    /**
     * @brief 에디터에서 프로그램 종료를 요청했는지 검사합니다.
     */
    bool Editor::FEditorUI::WantsExit() const {
        return mExit;
    }

    /**
     * @brief 가장 최근 입력을 받은 유효한 뷰포트를 반환합니다.
     */
    Editor::FEditorViewport* Editor::FEditorUI::GetActiveViewport() const {
        return mViewportLayout.GetActiveViewport();
    }

    /**
     * @brief 장면 로드 또는 저장용 Windows 파일 대화상자를 표시합니다.
     */
    std::filesystem::path Editor::FEditorUI::ChooseFile(bool Save) {
        std::array<wchar_t, 4096> Buffer{};

        if (Save) {
            const std::wstring Filename{mContext->GetScenePath().empty() ? L"Scene.masset" : mContext->GetScenePath().filename().wstring()};

            std::copy_n(Filename.begin(), std::min(Filename.size(), Buffer.size() - 1), Buffer.begin());
        }

        const std::filesystem::path Directory{mContext->GetProject()->GetAssets()->GetContentRoot() / "Maps"};

        std::error_code Error{};

        std::filesystem::create_directories(Directory, Error);

        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform->GetWindowHandle();
        Dialog.lpstrFilter = L"Macaw Scene (*.masset)\0*.masset\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.lpstrInitialDir = Directory.c_str();
        Dialog.lpstrDefExt = L"masset";
        Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | (Save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);

        if (Save ? GetSaveFileNameW(&Dialog) : GetOpenFileNameW(&Dialog)) {
            return std::filesystem::path{Buffer.data()};
        }

        return {};
    }

    /**
     * @brief 에디터 패널의 초기 ImGui 도킹 레이아웃을 구성합니다.
     */
    void Editor::FEditorUI::BuildLayout() {
        const ImGuiID Dockspace{ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport())};

        if (ImGui::DockBuilderGetNode(Dockspace) != nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[0] == nullptr && ImGui::DockBuilderGetNode(Dockspace)->ChildNodes[1] == nullptr) {
            ImGui::DockBuilderRemoveNode(Dockspace);
            ImGui::DockBuilderAddNode(Dockspace, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(Dockspace, ImGui::GetMainViewport()->WorkSize);

            ImGuiID Center{Dockspace};
            const ImGuiID Left{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Left, 0.19f, nullptr, &Center)};
            const ImGuiID Right{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Right, 0.32f, nullptr, &Center)};
            const ImGuiID Bottom{ImGui::DockBuilderSplitNode(Center, ImGuiDir_Down, 0.24f, nullptr, &Center)};

            ImGuiID Outliner{};
            const ImGuiID Placement{ImGui::DockBuilderSplitNode(Left, ImGuiDir_Down, 0.55f, nullptr, &Outliner)};

            ImGui::DockBuilderDockWindow(mPlaceActorsPanel.GetName().c_str(), Placement);
            ImGui::DockBuilderDockWindow(mOutlinerPanel.GetName().c_str(), Outliner);
            ImGui::DockBuilderDockWindow(mDetailsPanel.GetName().c_str(), Right);
            ImGui::DockBuilderDockWindow(mConsolePanel.GetName().c_str(), Bottom);
            ImGui::DockBuilderDockWindow(mSequencerPanel.GetName().c_str(), Bottom);
            ImGui::DockBuilderDockWindow(mAnimGraphPanel.GetName().c_str(), Bottom);
            ImGui::DockBuilderDockWindow("Content", Bottom);
            ImGui::DockBuilderDockWindow(mViewport->GetName().c_str(), Center);

            ImGui::DockBuilderFinish(Dockspace);
        }
    }

    /**
     * @brief 프로젝트·장면 작업과 편집 도구를 제공하는 툴바를 그립니다.
     */
    void Editor::FEditorUI::DrawEditorToolbar() {
        Editor::EditorStyle::PushViewportToolbar();

        const float Scale{mUIScale.GetScale()};

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{8.0f * Scale, 4.0f * Scale});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));

        const float Height{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f};
        const ImGuiWindowFlags Flags{ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing};

        if (ImGui::BeginViewportSideBar("##EditorToolbar", ImGui::GetMainViewport(), ImGuiDir_Up, Height, Flags)) {
            DrawPlayControls();
            ImGui::SameLine(0.0f, 12.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextDisabled("%s%s%s", mPlaySession->IsActive() ? mPlaySession->IsPaused() ? "[Paused] " : "[Playing] " : "", mContext->GetScenePath().empty() ? "Untitled" : Core::FilePath::ToUtf8(mContext->GetScenePath().filename()).c_str(), mContext->IsSceneDirty() ? " *" : "");

            if (mScriptBuilds.IsBuilding()) {
                ImGui::SameLine(0.0f, 16.0f);
                ImGui::TextDisabled("C# 빌드 중...");
            } else if (mScriptBuilds.HasCompileErrors()) {
                ImGui::SameLine(0.0f, 16.0f);
                ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "C# 컴파일 오류: 콘솔을 확인하세요");
            }
        }

        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);
        Editor::EditorStyle::PopViewportToolbar();
    }

    /**
     * @brief 변경된 C# 스크립트를 먼저 빌드한 뒤 PIE를 시작하도록 요청합니다.
     */
    void Editor::FEditorUI::RequestPlay() {
        if (mPlaySession->IsActive()) {
            return;
        }

        mScriptBuilds.BuildIfChanged();
        mPlayPending = true;

        UpdatePendingPlay();
    }

    /**
     * @brief 기다리던 빌드가 끝났으면 PIE를 시작합니다. 컴파일 오류가 있으면 시작하지 않습니다.
     */
    void Editor::FEditorUI::UpdatePendingPlay() {
        if (!mPlayPending || mScriptBuilds.IsBuilding()) {
            return;
        }

        mPlayPending = false;

        if (mScriptBuilds.HasCompileErrors()) {
            mContext->AddLog("PIE canceled: fix the C# compile errors first");
        } else if (mViewportLayout.CanChangeLayout()) {
            mPlaySession->Start(&mPlatform->GetInput());
        }
    }

    /**
     * @brief PIE 시작·일시정지·단계 실행·종료 버튼을 그립니다.
     */
    void Editor::FEditorUI::DrawPlayControls() {
        ImGui::PushID("PIEControls");

        const bool Active{mPlaySession->IsActive()};
        const bool Paused{mPlaySession->IsPaused()};
        const bool CanStart{!Active && mViewportLayout.CanChangeLayout() && mPlaySession->CanStart()};
        const char* PlayTooltip{Active ? "중지 (Esc)" : mPlayPending ? "C# 빌드를 기다리는 중 (클릭하면 취소)" : CanStart ? "플레이 (Alt+P)" : "플레이하려면 활성 월드 카메라가 있고 에디터가 다른 작업 중이 아니어야 합니다"};

        if (Editor::EditorStyle::IconButton("PlayStop", Active ? Editor::EditorStyle::EIcon::Stop : Editor::EditorStyle::EIcon::Play, mPlayPending, Active || CanStart, false, PlayTooltip)) {
            if (Active) {
                mPlaySession->Stop();
            } else if (mPlayPending) {
                mPlayPending = false;
            } else {
                RequestPlay();
            }
        }

        ImGui::SameLine(0.0f, 4.0f);

        if (Editor::EditorStyle::IconButton("PauseResume", Paused ? Editor::EditorStyle::EIcon::Play : Editor::EditorStyle::EIcon::Pause, Paused, Active, false, Paused ? "재개" : "일시 정지")) {
            mPlaySession->SetPaused(!Paused);
        }

        ImGui::SameLine(0.0f, 4.0f);

        if (Editor::EditorStyle::IconButton("Step", Editor::EditorStyle::EIcon::Step, false, Active && Paused, false, "1틱 진행")) {
            mPlaySession->Step();
        }

        ImGui::PopID();
    }

    /**
     * @brief 장면 파일과 패널 표시 메뉴를 그립니다.
     */
    void Editor::FEditorUI::DrawMenu() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::BeginDisabled(!mContext->CanEditWorld());

                if (ImGui::MenuItem("New Project...")) {
                    ChooseProject(true);
                }

                if (ImGui::MenuItem("Open Project...")) {
                    ChooseProject(false);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    if (ConfirmSceneChange()) {
                        mContext->NewScene();
                    }
                }

                if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
                    const auto Path{ChooseFile(false)};

                    if (!Path.empty() && ConfirmSceneChange()) {
                        mContext->LoadScene(Path);
                    }
                }

                const bool CanSave{!mContext->IsSceneEditActive()};

                if (ImGui::MenuItem("Save Scene", "Ctrl+S", false, CanSave)) {
                    const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

                    if (!Path.empty()) {
                        mContext->SaveScene(Path);
                        mRefreshContent = true;
                    }
                }

                if (ImGui::MenuItem("Save Scene As...", nullptr, false, CanSave)) {
                    const auto Path{ChooseFile(true)};

                    if (!Path.empty()) {
                        mContext->SaveScene(Path);
                        mRefreshContent = true;
                    }
                }

                if (ImGui::MenuItem("Set Current Scene as Startup", nullptr, false, CanSave && !mContext->GetScenePath().empty())) {
                    Core::FString Error{};

                    if (!mContext->SaveScene(mContext->GetScenePath()) || !mContext->GetProject()->SetStartupScene(mContext->GetScenePath(), Error)) {
                        mContext->AddLog(Error);
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Export Game...", nullptr, false, mContext->GetProject() != nullptr)) {
                    ExportGame();
                }

                ImGui::EndDisabled();
                ImGui::Separator();

                if (ImGui::MenuItem("Exit") && ConfirmSceneChange()) {
                    mExit = true;
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, mContext->CanUndo())) {
                    mContext->Undo();
                }

                if (ImGui::MenuItem("Redo", "Ctrl+Y / Ctrl+Shift+Z", false, mContext->CanRedo())) {
                    mContext->Redo();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Statistics", nullptr, &mShowStatistics);

                if (ImGui::MenuItem("Anim Graph", nullptr, mAnimGraphPanel.IsVisible())) {
                    if (mAnimGraphPanel.IsVisible()) {
                        mAnimGraphPanel.SetVisible(false);
                    } else {
                        mAnimGraphPanel.Open();
                    }
                }

                if (ImGui::BeginMenu("Stat Overlay")) {
                    Editor::FViewportStatistics* Statistics{GetActiveViewport()->GetStatistics()};

                    for (const Core::FString& Group : Core::FStats::Get().GetGroups()) {
                        if (ImGui::MenuItem(Group.c_str(), nullptr, Statistics->IsEnabled(Group))) {
                            Core::FString Output{};

                            Statistics->ExecuteCommand("stat " + Group, Output);
                        }
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Scripts", mContext->GetProject() != nullptr)) {
                if (ImGui::MenuItem("C# 프로젝트 열기")) {
                    mScriptBuilds.OpenProject();
                }

                if (ImGui::MenuItem("스크립트 빌드", nullptr, false, mScriptBuilds.HasProject() && !mScriptBuilds.IsBuilding())) {
                    mScriptBuilds.Rebuild();
                }

                ImGui::Separator();
                ImGui::TextDisabled("C# 디버깅: Visual Studio의 프로세스에 연결에서");
                ImGui::TextDisabled("Macaw.exe를 고르고 코드 형식을 Managed(.NET Core)로 지정합니다.");
                ImGui::EndMenu();
            }

            DrawSettingsMenu();
            ImGui::EndMainMenuBar();
        }

        using Core::EKey;

        const Core::FInput& Input{mPlatform->GetInput()};

        if (Input.IsDown(EKey::Control) && !ImGui::GetIO().WantTextInput && Input.WasPressed(EKey::Comma)) {
            mSettingsOpen = true;
            mFocusSettings = true;
        }

        if (!ImGui::GetIO().WantTextInput && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
            if (Input.IsDown(EKey::Alt) && Input.WasPressed(EKey::P) && mViewportLayout.CanChangeLayout()) {
                RequestPlay();
            } else if (mPlaySession->IsActive() && Input.WasPressed(EKey::Escape)) {
                mPlaySession->Stop();
            }
        }

        if (Input.IsDown(EKey::Control) && !ImGui::GetIO().WantTextInput && mContext->CanEditWorld()) {
            const ImGuiWindow* Focused{ImGui::GetCurrentContext()->NavWindow};
            const ImGuiWindow* Root{Focused != nullptr ? Focused->RootWindow : nullptr};
            const bool SceneFocused{Root != nullptr && (Root == ImGui::FindWindowByName(mViewport->GetName().c_str()) || Root == ImGui::FindWindowByName(mOutlinerPanel.GetName().c_str()) || Root == ImGui::FindWindowByName(mDetailsPanel.GetName().c_str()) || Root == ImGui::FindWindowByName(mPlaceActorsPanel.GetName().c_str()))};

            if (SceneFocused && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup)) {
                if (Input.WasPressed(EKey::Y) || (Input.WasPressed(EKey::Z) && Input.IsDown(EKey::Shift))) {
                    mContext->Redo();
                } else if (Input.WasPressed(EKey::Z)) {
                    mContext->Undo();
                }
            }

            if (Input.WasPressed(EKey::S) && (mAnimGraphEditor == nullptr || !mAnimGraphEditor->IsFocused()) && (mBlendSpaceEditor == nullptr || !mBlendSpaceEditor->IsFocused())) {
                const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};

                if (!Path.empty()) {
                    mContext->SaveScene(Path);
                    mRefreshContent = true;
                }
            } else if (Input.WasPressed(EKey::O)) {
                const auto Path{ChooseFile(false)};

                if (!Path.empty() && ConfirmSceneChange()) {
                    mContext->LoadScene(Path);
                }
            } else if (Input.WasPressed(EKey::N)) {
                if (ConfirmSceneChange()) {
                    mContext->NewScene();
                }
            }
        }
    }

    /**
     * @brief 내보낼 폴더를 고르고 저장하지 않은 장면을 확인한 뒤 현재 프로젝트를 독립 실행 게임으로 내보냅니다.
     */
    void Editor::FEditorUI::ExportGame() {
        const auto Destination{Editor::FProjectDialog::ChooseDirectory(mPlatform->GetWindowHandle())};

        if (Destination.empty()) {
            return;
        }

        if (!ConfirmSceneChange()) {
            return;
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        Core::FString Error{};
        bool Exported{};

        if (Length == 0 || Length >= Executable.size()) {
            Error = "Cannot locate the game executable";
        } else {
            const auto Runtime{std::filesystem::path{Executable.data()}.parent_path() / "GameRuntime"};

            Exported = Editor::FGameExporter::Export(*mContext->GetProject(), Runtime, Destination, &mScriptBuilds, Error);
        }

        const Core::FString Message{Exported ? "Exported game to " + Core::FilePath::ToUtf8(Destination) : Error};

        mContext->AddLog(Message);
        MessageBoxW(mPlatform->GetWindowHandle(), Core::FilePath::FromUtf8(Message).c_str(), Exported ? L"Game Exported" : L"Cannot Export Game", MB_OK | (Exported ? MB_ICONINFORMATION : MB_ICONERROR));
    }

    /**
     * @brief 저장하지 않은 장면 변경을 확인하고 전환을 계속할 수 있는지 판정합니다.
     */
    bool Editor::FEditorUI::ConfirmSceneChange() {
        if (!ConfirmMaterialChange()) {
            return false;
        }

        if (!mContext->IsSceneDirty()) {
            return true;
        }

        const Core::Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current scene?", L"Scene", MB_YESNOCANCEL | MB_ICONQUESTION)};

        if (Choice == IDCANCEL) {
            return false;
        }

        if (Choice == IDNO) {
            return true;
        }

        const auto Path{mContext->GetScenePath().empty() ? ChooseFile(true) : mContext->GetScenePath()};
        const bool Saved{!Path.empty() && mContext->SaveScene(Path)};

        mRefreshContent = mRefreshContent || Saved;

        return Saved;
    }

    /**
     * @brief 프로젝트 파일 대화상자를 열고 선택한 경로의 전환을 요청합니다.
     */
    void Editor::FEditorUI::ChooseProject(bool Create) {
        if (!ConfirmSceneChange()) {
            return;
        }

        mContext->RequestProject({}, Create);
    }

    /**
     * @brief 파일 대화상자에서 고른 메시 또는 이미지를 프로젝트 에셋으로 가져옵니다.
     */
    void Editor::FEditorUI::ImportAsset() {
        std::array<wchar_t, 32768> Buffer{};
        OPENFILENAMEW Dialog{};
        Dialog.lStructSize = sizeof(Dialog);
        Dialog.hwndOwner = mPlatform->GetWindowHandle();
        Dialog.lpstrTitle = L"Import Asset";
        Dialog.lpstrFilter = L"Supported Assets\0*.obj;*.fbx;*.gltf;*.glb;*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr\0";
        Dialog.lpstrFile = Buffer.data();
        Dialog.nMaxFile = static_cast<DWORD>(Buffer.size());
        Dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        if (GetOpenFileNameW(&Dialog)) {
            RequestImport({std::filesystem::path{Buffer.data()}}, mContentDirectory);
        }
    }

    /**
     * @brief 원본 파일 목록을 가져오기 요청합니다. 메시가 포함되어 있으면 임포트 옵션 창을 띄우고, 이미지뿐이면 바로 가져옵니다.
     */
    void Editor::FEditorUI::RequestImport(Core::TArray<std::filesystem::path> Sources, const std::filesystem::path& Directory) {
        if (!mContext->CanEditWorld() || Sources.empty()) {
            return;
        }

        const bool HasMesh{std::any_of(Sources.begin(), Sources.end(), [](const std::filesystem::path& Source) {
            return Asset::IMeshImporter::IsSupported(Source);
        })};

        // 이미지 원본만 있으면 받을 옵션이 없으므로 바로 가져옵니다.
        if (!HasMesh) {
            ImportFiles(Sources, Directory, mImportOption);

            return;
        }

        mPendingImports.push_back(std::move(Sources));
        mPendingImportDirectories.push_back(Directory);
    }

    /**
     * @brief 대기 중인 메시 가져오기 묶음이 있으면 임포트 옵션 모달 창을 그리고, 확인하면 옵션대로 가져옵니다.
     */
    void Editor::FEditorUI::DrawImportOptions() {
        static constexpr const char* PopupName{"Import Options"};

        if (mPendingImports.empty()) {
            return;
        }

        if (!ImGui::IsPopupOpen(PopupName)) {
            ImGui::OpenPopup(PopupName);
        }

        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, {0.5f, 0.5f});

        if (!ImGui::BeginPopupModal(PopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        const auto& Sources{mPendingImports.front()};
        const auto Finish{[this]() {
            mPendingImports.erase(mPendingImports.begin());
            mPendingImportDirectories.erase(mPendingImportDirectories.begin());
            ImGui::CloseCurrentPopup();
        }};

        constexpr std::size_t ListedSources{8};

        ImGui::TextUnformatted(Sources.size() == 1 ? "Importing 1 file:" : ("Importing " + std::to_string(Sources.size()) + " files:").c_str());

        for (std::size_t Index{}; Index < std::min(Sources.size(), ListedSources); ++Index) {
            ImGui::BulletText("%s", Core::FilePath::ToUtf8(Sources[Index].filename()).c_str());
        }

        if (Sources.size() > ListedSources) {
            ImGui::TextDisabled("... and %zu more", Sources.size() - ListedSources);
        }

        if (mPendingImports.size() > 1) {
            ImGui::TextDisabled("%zu more import batches are waiting.", mPendingImports.size() - 1);
        }

        ImGui::SeparatorText("Transform");
        ImGui::Checkbox("Force Front X Axis", &mImportOption.mForceFrontXAxis);
        ImGui::SetItemTooltip("원본의 앞쪽 축이 +X를 향하도록 맞춥니다.\n끄면 언리얼 FBX 임포터와 같이 앞쪽이 +Y를 향합니다.");
        ImGui::Checkbox("Convert Scene Unit", &mImportOption.mConvertSceneUnit);
        ImGui::SetItemTooltip("원본(FBX)에 기록된 길이 단위를 미터로 변환합니다.");

        std::array<float, 3> Translation{mImportOption.mImportTranslation.x, mImportOption.mImportTranslation.y, mImportOption.mImportTranslation.z};
        std::array<float, 3> Rotation{mImportOption.mImportRotation.pitch, mImportOption.mImportRotation.yaw, mImportOption.mImportRotation.roll};

        if (ImGui::DragFloat3("Import Translation", Translation.data(), 0.01f, 0.0f, 0.0f, "%.3f m")) {
            mImportOption.mImportTranslation = {Translation[0], Translation[1], Translation[2]};
        }

        if (ImGui::DragFloat3("Import Rotation", Rotation.data(), 0.5f, -360.0f, 360.0f, "%.1f deg")) {
            mImportOption.mImportRotation = {Rotation[0], Rotation[1], Rotation[2]};
        }

        ImGui::SetItemTooltip("Pitch, Yaw, Roll 순서의 각도(도)입니다.");
        ImGui::DragFloat("Import Uniform Scale", &mImportOption.mImportUniformScale, 0.01f, 0.001f, 1000.0f, "%.3f");

        ImGui::SeparatorText("Mesh");
        ImGui::Checkbox("Import Mesh", &mImportOption.mImportMesh);
        ImGui::SetItemTooltip("메시와 재질을 가져옵니다.\n끄면 스켈레톤이나 애니메이션만 가져옵니다.");
        ImGui::BeginDisabled(!mImportOption.mImportMesh);
        ImGui::Checkbox("Combine Meshes", &mImportOption.mCombineMeshes);
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("파일의 모든 메시를 하나의 메시 에셋으로 합칩니다.\n끄면 메시 노드마다 별도의 메시 에셋을 만듭니다.");
        ImGui::Checkbox("Import Skeleton", &mImportOption.mImportSkeleton);
        ImGui::SetItemTooltip("원본에 스켈레톤이 있으면 본 계층, 바인드 포즈, 정점별 본 가중치를 읽습니다.\n메시는 스켈레탈 메시 에셋으로 저장되며 배치하거나 메시 뷰어에서 열 수 있습니다.");

        static constexpr std::array<const char*, 3> NormalMethods{"Compute Normals", "Import Normals", "Import Normals and Tangents"};
        int NormalMethod{static_cast<int>(mImportOption.mNormalImportMethod)};

        if (ImGui::Combo("Normal Import Method", &NormalMethod, NormalMethods.data(), static_cast<int>(NormalMethods.size()))) {
            mImportOption.mNormalImportMethod = static_cast<Asset::ENormalImportMethod>(NormalMethod);
        }

        ImGui::SeparatorText("Material");
        ImGui::Checkbox("Import Materials", &mImportOption.mImportMaterials);
        ImGui::SetItemTooltip("원본 재질로 머티리얼 에셋을 만듭니다.\n끄면 메시 슬롯이 기본 머티리얼을 사용합니다.");
        ImGui::BeginDisabled(!mImportOption.mImportMaterials);
        ImGui::Checkbox("Import Textures", &mImportOption.mImportTextures);
        ImGui::EndDisabled();

        ImGui::SeparatorText("Animation");
        ImGui::Checkbox("Import Animations", &mImportOption.mImportAnimations);
        ImGui::SetItemTooltip("FBX의 애니메이션 스택마다 본 트랙을 재샘플링해 애니메이션 시퀀스 에셋을 만듭니다.\n애니메이션을 적용할 스켈레톤을 임포트할 때와 같은 축, 단위, 변환 옵션을 사용하세요.");
        ImGui::BeginDisabled(!mImportOption.mImportAnimations);

        static constexpr std::array<const char*, 3> AnimationLengths{"Exported Time", "Animated Key", "Set Range"};
        int AnimationLength{static_cast<int>(mImportOption.mAnimationLength)};

        if (ImGui::Combo("Animation Length", &AnimationLength, AnimationLengths.data(), static_cast<int>(AnimationLengths.size()))) {
            mImportOption.mAnimationLength = static_cast<Asset::EAnimationLengthImportType>(AnimationLength);
        }

        ImGui::SetItemTooltip("Exported Time: 애니메이션 스택에 기록된 시간 구간을 사용합니다.\nAnimated Key: 애니메이션 키가 있는 구간을 사용합니다.\nSet Range: 아래에 지정한 프레임 구간을 사용합니다.");

        if (mImportOption.mAnimationLength == Asset::EAnimationLengthImportType::SetRange) {
            std::array<int, 2> Range{mImportOption.mFrameImportRangeStart, mImportOption.mFrameImportRangeEnd};

            if (ImGui::DragInt2("Frame Import Range", Range.data(), 1.0f)) {
                mImportOption.mFrameImportRangeStart = Range[0];
                mImportOption.mFrameImportRangeEnd = Range[1];
            }
        }

        ImGui::DragInt("Custom Sample Rate", &mImportOption.mCustomSampleRate, 1.0f, 0, 1000, mImportOption.mCustomSampleRate == 0 ? "Source" : "%d fps");
        ImGui::SetItemTooltip("0이면 원본 장면의 프레임 레이트를 사용합니다(없으면 30 fps).");
        ImGui::Checkbox("Snap To Closest Frame Boundary", &mImportOption.mSnapToClosestFrameBoundary);
        ImGui::SetItemTooltip("시작과 끝 시간을 가장 가까운 프레임 경계에 맞춥니다.");
        ImGui::Checkbox("Remove Redundant Keys", &mImportOption.mRemoveRedundantKeys);
        ImGui::SetItemTooltip("값이 변하지 않는 위치, 회전, 스케일 채널은 키 하나만 저장합니다.");
        ImGui::EndDisabled();

        if (std::any_of(Sources.begin(), Sources.end(), [](const std::filesystem::path& Source) {
                return !Asset::IMeshImporter::IsSupported(Source);
            })) {
            ImGui::TextDisabled("Image files in this batch are imported without these options.");
        }

        ImGui::Separator();

        const bool Valid{mImportOption.IsValid()};

        ImGui::BeginDisabled(!Valid);

        if (ImGui::Button("Import")) {
            const auto Batch{Sources};
            const auto Directory{mPendingImportDirectories.front()};

            Finish();
            ImportFiles(Batch, Directory, mImportOption);
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::Button("Reset to Defaults")) {
            mImportOption = Asset::FAssetImportOption{};
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            mContext->AddLog("Import canceled");
            Finish();
        }

        if (!Valid) {
            ImGui::TextColored({1.0f, 0.4f, 0.4f, 1.0f}, "Translation, rotation and scale must be finite, and scale must not be zero.");
        }

        ImGui::EndPopup();
    }

    /**
     * @brief 원본 파일 목록을 대상 콘텐츠 디렉터리에 가져오고 에셋 목록과 결과 로그를 갱신합니다.
     */
    void Editor::FEditorUI::ImportFiles(const Core::TArray<std::filesystem::path>& Sources, const std::filesystem::path& Directory, const Asset::FAssetImportOption& Option) {
        if (!mContext->CanEditWorld()) {
            return;
        }

        const std::filesystem::path Destination{Directory};
        Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
        std::filesystem::path LastImported{};
        Core::FString Failures{};
        Core::Uint32 Imported{};

        for (const auto& Source : Sources) {
            std::filesystem::path Target{};
            Core::FString Error{};

            if (!Assets->ImportAsset(Source, Destination, Option, Target, Error)) {
                const Core::FString Failure{"Cannot import '" + Core::FilePath::ToUtf8(Source.filename()) + "': " + Error};

                mContext->AddLog(Failure);

                Failures += (Failures.empty() ? Core::FString{} : "\n") + Failure;

                continue;
            }

            ++Imported;
            LastImported = Target;

            mContext->AddLog("Imported " + Core::FilePath::ToGenericUtf8(Target));
        }

        if (Imported != 0) {
            mContentFilter.reset();
            mContentSearch.fill('\0');
            RefreshContent();
            NavigateContent(Imported == 1 ? LastImported.parent_path() : Destination);

            mSelectedContentPath = Imported == 1 ? LastImported : LastImported.parent_path();
        }

        mContentError = std::move(Failures);
    }

    /**
     * @brief Windows 드롭 핸들에서 경로와 대상 영역을 수집하여 가져오기를 대기열에 넣고 핸들을 해제합니다.
     */
    void Editor::FEditorUI::QueueFileDrop(void* Handle) {
        const HDROP Drop{static_cast<HDROP>(Handle)};
        POINT Point{};

        if (Drop == nullptr) {
            return;
        }

        ImGuiContext* Context{ImGui::GetCurrentContext()};

        if (!mInitialized || !mContext->CanEditWorld() || Context == nullptr || !DragQueryPoint(Drop, &Point) || Context->OpenPopupStack.Size != 0) {
            DragFinish(Drop);

            return;
        }

        if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
            ClientToScreen(mPlatform->GetWindowHandle(), &Point);
        }

        const ImVec2 Position{static_cast<float>(Point.x), static_cast<float>(Point.y)};
        ImGuiWindow* Browser{ImGui::FindWindowByName("Content")};
        ImGuiWindow* Hovered{};

        for (int Index{Context->Windows.Size}; Index > 0; --Index) {
            ImGuiWindow* Window{Context->Windows[Index - 1]};

            if (Window->WasActive && !Window->Hidden && (Window->Flags & ImGuiWindowFlags_NoMouseInputs) == 0 && Window->OuterRectClipped.Contains(Position)) {
                Hovered = Window;

                break;
            }
        }

        if (Browser == nullptr || Browser->Collapsed || Hovered == nullptr || !ImGui::IsWindowChildOf(Hovered, Browser, false, false)) {
            DragFinish(Drop);

            return;
        }

        std::filesystem::path Directory{};
        bool Found{};

        for (std::size_t Index{mContentDropBounds.size()}; Index > 0; --Index) {
            const Core::FVector4& Bounds{mContentDropBounds[Index - 1]};

            if (Position.x >= Bounds.x && Position.y >= Bounds.y && Position.x < Bounds.z && Position.y < Bounds.w) {
                Directory = mContentDropDirectories[Index - 1];
                Found = true;

                break;
            }
        }

        if (!Found) {
            DragFinish(Drop);

            return;
        }

        Core::TArray<std::filesystem::path> Sources{};
        const UINT Count{DragQueryFileW(Drop, 0xFFFFFFFF, nullptr, 0)};

        for (UINT Index{}; Index < Count; ++Index) {
            const UINT Length{DragQueryFileW(Drop, Index, nullptr, 0)};
            std::wstring Path{};
            Path.resize(static_cast<std::size_t>(Length) + 1);

            if (Length != 0 && DragQueryFileW(Drop, Index, Path.data(), Length + 1) == Length) {
                Path.resize(Length);
                Sources.emplace_back(Path);
            }
        }

        DragFinish(Drop);

        if (!Sources.empty()) {
            mDropDirectories.push_back(std::move(Directory));
            mDroppedFiles.push_back(std::move(Sources));
        }
    }

    /**
     * @brief 대기 중인 파일 드롭 묶음을 꺼내 각 대상 디렉터리로 가져옵니다.
     */
    void Editor::FEditorUI::ImportDroppedFiles() {
        auto Files{std::move(mDroppedFiles)};
        auto Directories{std::move(mDropDirectories)};

        mDroppedFiles.clear();
        mDropDirectories.clear();

        for (std::size_t Index{}; Index < Files.size(); ++Index) {
            RequestImport(std::move(Files[Index]), Directories[Index]);
        }
    }

    /**
     * @brief 콘텐츠 영역을 현재 ImGui 클립 사각형으로 잘라 파일 드롭 대상으로 등록합니다.
     */
    void Editor::FEditorUI::RegisterContentDropTarget(const std::filesystem::path& Directory, const Core::FVector4& Bounds) {
        const ImRect Clip{ImGui::GetCurrentWindow()->ClipRect};
        const Core::FVector4 Visible{std::max(Bounds.x, Clip.Min.x), std::max(Bounds.y, Clip.Min.y), std::min(Bounds.z, Clip.Max.x), std::min(Bounds.w, Clip.Max.y)};

        if (Visible.z > Visible.x && Visible.w > Visible.y) {
            mContentDropDirectories.push_back(Directory);
            mContentDropBounds.push_back(Visible);
        }
    }

    /**
     * @brief 프로젝트 에셋 목록과 생성·가져오기 UI를 그립니다.
     */
    void Editor::FEditorUI::DrawContent() {
        mContentDropDirectories.clear();
        mContentDropBounds.clear();

        if (ImGui::Begin("Content")) {
            const ImVec2 Minimum{ImGui::GetWindowPos()};
            const ImVec2 Size{ImGui::GetWindowSize()};

            RegisterContentDropTarget(mContentDirectory, {Minimum.x, Minimum.y, Minimum.x + Size.x, Minimum.y + Size.y});
            ImGui::TextUnformatted(mContext->GetProject()->GetName().c_str());
            ImGui::SameLine();

            if (ImGui::SmallButton("Refresh")) {
                mRefreshContent = true;
            }

            ImGui::Separator();
            DrawContentBreadcrumbs();
            ImGui::SetNextItemWidth(120.0f);

            const char* FilterName{!mContentFilter.has_value() ? "All Assets" : *mContentFilter == Asset::EAssetType::Mesh ? "Meshes" : *mContentFilter == Asset::EAssetType::SkeletalMesh ? "Skeletal Meshes" : *mContentFilter == Asset::EAssetType::Skeleton ? "Skeletons" : *mContentFilter == Asset::EAssetType::Material ? "Materials" : *mContentFilter == Asset::EAssetType::Texture ? "Textures" : *mContentFilter == Asset::EAssetType::Sequence ? "Sequences" : *mContentFilter == Asset::EAssetType::AnimSequence ? "Animation Sequences" : *mContentFilter == Asset::EAssetType::BlendSpace ? "Blend Spaces" : *mContentFilter == Asset::EAssetType::AnimGraph ? "Anim Graphs" : "Scenes"};

            if (ImGui::BeginCombo("##AssetType", FilterName)) {
                if (ImGui::Selectable("All Assets", !mContentFilter.has_value())) {
                    mContentFilter.reset();
                }

                for (Asset::EAssetType Type : {Asset::EAssetType::Mesh, Asset::EAssetType::SkeletalMesh, Asset::EAssetType::Skeleton, Asset::EAssetType::Material, Asset::EAssetType::Texture, Asset::EAssetType::Scene, Asset::EAssetType::Sequence, Asset::EAssetType::AnimSequence, Asset::EAssetType::BlendSpace, Asset::EAssetType::AnimGraph}) {
                    const char* Name{Type == Asset::EAssetType::Mesh ? "Meshes" : Type == Asset::EAssetType::SkeletalMesh ? "Skeletal Meshes" : Type == Asset::EAssetType::Skeleton ? "Skeletons" : Type == Asset::EAssetType::Material ? "Materials" : Type == Asset::EAssetType::Texture ? "Textures" : Type == Asset::EAssetType::Sequence ? "Sequences" : Type == Asset::EAssetType::AnimSequence ? "Animation Sequences" : Type == Asset::EAssetType::BlendSpace ? "Blend Spaces" : Type == Asset::EAssetType::AnimGraph ? "Anim Graphs" : "Scenes"};

                    if (ImGui::Selectable(Name, mContentFilter == Type)) {
                        mContentFilter = Type;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(std::max(80.0f, ImGui::GetContentRegionAvail().x));
            ImGui::InputTextWithHint("##ContentSearch", "Search assets...", mContentSearch.data(), mContentSearch.size());

            if (!mContentError.empty()) {
                ImGui::TextWrapped("%s", mContentError.c_str());
            }

            // 테이블 행의 위아래 셀 여백만큼 줄여 창 스크롤바가 생기지 않게 합니다.
            const float Height{std::max(64.0f, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().CellPadding.y * 2.0f)};

            if (ImGui::BeginTable("Content Browser", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 190.0f);
                ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::BeginChild("Content Folders", ImVec2{0.0f, Height});
                DrawContentTree({});

                // C# 스크립트는 Content 밖의 Scripts 폴더에 있으므로 별도 루트로 보여 줍니다.
                if (mScriptBuilds.HasProject()) {
                    const ImGuiTreeNodeFlags ScriptFlags{ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen | (mBrowsingScripts ? ImGuiTreeNodeFlags_Selected : 0)};

                    ImGui::TreeNodeEx("Scripts##ContentScripts", ScriptFlags);

                    if (ImGui::IsItemClicked()) {
                        mBrowsingScripts = true;
                        mSelectedContentPath.clear();
                    }
                } else {
                    mBrowsingScripts = false;
                }

                mRevealContentDirectory = false;

                if (ImGui::BeginPopupContextWindow("ContentFolderMenu")) {
                    DrawContentContextMenu();
                    ImGui::EndPopup();
                }

                ImGui::EndChild();
                ImGui::TableSetColumnIndex(1);
                ImGui::BeginChild("Content Files", ImVec2{0.0f, Height});
                DrawContentEntries();

                if (ImGui::IsWindowFocused() && !ImGui::GetIO().WantTextInput && !mSelectedContentPath.empty() && mContext->CanEditWorld()) {
                    if (ImGui::IsKeyPressed(ImGuiKey_F2, false)) {
                        BeginContentAction(EContentAction::Rename);
                    } else if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
                        BeginContentAction(EContentAction::Delete);
                    }
                }

                if (ImGui::BeginPopupContextWindow("ContentFileMenu")) {
                    DrawContentContextMenu();
                    ImGui::EndPopup();
                }

                ImGui::EndChild();
                ImGui::EndTable();
            }

            DrawNewScriptPopup();
            DrawContentActionPopup();
        }

        ImGui::End();
    }

    /**
     * @brief 콘텐츠 항목 경로를 디스크 경로로 바꿉니다.
     */
    std::filesystem::path Editor::FEditorUI::GetContentActionPath(const std::filesystem::path& Path) const {
        return mBrowsingScripts ? Path : mContext->GetProject()->GetAssets()->GetContentRoot() / Path;
    }

    /**
     * @brief 선택한 항목에 대한 작업 창을 열고, 이 항목을 참조하는 에셋을 찾아 둡니다.
     */
    void Editor::FEditorUI::BeginContentAction(EContentAction Action) {
        mContentAction = Action;
        mContentActionTarget = mSelectedContentPath;
        mContentActionScript = mBrowsingScripts;
        mContentActionError.clear();
        mContentActionReferences.clear();
        mOpenContentAction = true;

        const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
        std::error_code Code{};
        const bool Directory{std::filesystem::is_directory(Absolute, Code)};
        const Core::FString Name{Core::FilePath::ToUtf8(Directory || mContentActionScript ? mContentActionTarget.filename() : mContentActionTarget.stem())};

        mContentActionName.fill('\0');
        std::copy_n(Name.begin(), std::min(Name.size(), mContentActionName.size() - 1), mContentActionName.begin());

        // 에셋은 다른 에셋에 상대 경로 문자열로 저장되고, 스크립트는 씬에 클래스 이름으로 저장됩니다.
        Core::FString Needle{};
        std::regex ScriptClass{};

        if (mContentActionScript) {
            ScriptClass = std::regex{"\"ScriptClass\":\\s*\"([A-Za-z0-9_.]*\\.)?" + Core::FilePath::ToUtf8(mContentActionTarget.stem()) + "\""};
        } else {
            Needle = Core::FilePath::ToGenericUtf8(mContentActionTarget) + (Directory ? "/" : "\"");
            Needle = Directory ? "\"" + Needle : "\"" + Needle;
        }

        const std::filesystem::path Root{mContext->GetProject()->GetAssets()->GetContentRoot()};
        std::filesystem::recursive_directory_iterator Iterator{Root, std::filesystem::directory_options::skip_permission_denied, Code};
        const std::filesystem::recursive_directory_iterator End{};

        for (; !Code && Iterator != End; Iterator.increment(Code)) {
            const std::filesystem::path Relative{Iterator->path().lexically_relative(Root)};
            const auto Inside{Relative.lexically_relative(mContentActionTarget)};

            if (!Iterator->is_regular_file(Code) || Core::FilePath::ToLowerExtension(Relative) != L".masset" || (!mContentActionScript && (Relative == mContentActionTarget || (!Inside.empty() && *Inside.begin() != L"..")))) {
                continue;
            }

            std::ifstream Input{Iterator->path(), std::ios::binary};
            const std::string Text{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}};

            if (mContentActionScript ? std::regex_search(Text, ScriptClass) : Text.find(Needle) != std::string::npos) {
                mContentActionReferences.push_back(Relative);
            }
        }
    }

    /**
     * @brief 열려 있는 씬이나 편집 중인 머티리얼처럼 지금 바꾸면 안 되는 이유를 찾습니다.
     */
    Core::FString Editor::FEditorUI::FindContentActionBlocker(const std::filesystem::path& Absolute) const {
        Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
        Core::FString Error{};
        std::error_code Code{};
        const auto Contains{[&](const std::filesystem::path& Open) {
            if (Open.empty()) {
                return false;
            }

            const std::filesystem::path Resolved{Assets->ResolvePath(Open, Error)};
            const auto Relative{std::filesystem::weakly_canonical(Resolved, Code).lexically_relative(std::filesystem::weakly_canonical(Absolute, Code))};

            return !Resolved.empty() && !Relative.empty() && *Relative.begin() != L"..";
        }};

        if (mPlaySession->IsActive() || !mContext->CanEditWorld()) {
            return "PIE 중에는 바꿀 수 없습니다.";
        }

        if (Contains(mContext->GetScenePath())) {
            return "지금 열려 있는 씬입니다. 다른 씬을 연 뒤에 다시 시도하세요.";
        }

        if (mEditingMaterial != nullptr && Contains(mMaterialPath)) {
            return "편집 중인 머티리얼입니다. 머티리얼 편집기를 닫은 뒤에 다시 시도하세요.";
        }

        return {};
    }

    /**
     * @brief 이름 바꾸기 또는 삭제 확인 창을 표시합니다.
     */
    void Editor::FEditorUI::DrawContentActionPopup() {
        constexpr const char* RenameName{"Rename Asset"};
        constexpr const char* DeleteName{"Delete Asset"};
        const char* PopupName{mContentAction == EContentAction::Rename ? RenameName : DeleteName};

        if (mOpenContentAction) {
            ImGui::OpenPopup(PopupName);

            mOpenContentAction = false;
        }

        if (!ImGui::BeginPopupModal(PopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
        std::error_code Code{};
        const bool Directory{std::filesystem::is_directory(Absolute, Code)};
        const Core::FString Display{mContentActionScript ? Core::FilePath::ToUtf8(mContentActionTarget.filename()) : Core::FilePath::ToGenericUtf8(mContentActionTarget)};
        bool Confirmed{};

        if (mContentAction == EContentAction::Rename) {
            ImGui::Text("%s의 새 이름", Display.c_str());

            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }

            Confirmed = ImGui::InputText("##ContentRename", mContentActionName.data(), mContentActionName.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

            if (mContentActionScript) {
                ImGui::TextDisabled("파일 안의 클래스 이름도 함께 바꿉니다.");
            }
        } else {
            ImGui::Text("%s%s을(를) 휴지통으로 옮길까요?", Display.c_str(), Directory ? " 폴더와 그 안의 모든 파일" : "");
        }

        if (!mContentActionReferences.empty()) {
            ImGui::Spacing();
            ImGui::TextColored({1.0f, 0.75f, 0.3f, 1.0f}, mContentActionScript ? "다음 씬이 이 스크립트 클래스를 씁니다. 연결이 끊어집니다." : "다음 에셋이 이 경로를 참조합니다. 참조가 끊어집니다.");

            for (std::size_t Index{}; Index < std::min<std::size_t>(mContentActionReferences.size(), 6); ++Index) {
                ImGui::BulletText("%s", Core::FilePath::ToGenericUtf8(mContentActionReferences[Index]).c_str());
            }

            if (mContentActionReferences.size() > 6) {
                ImGui::TextDisabled("외 %zu개", mContentActionReferences.size() - 6);
            }
        }

        if (!mContentActionError.empty()) {
            ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", mContentActionError.c_str());
        }

        ImGui::Spacing();

        Confirmed |= ImGui::Button(mContentAction == EContentAction::Rename ? "Rename" : "Delete");

        ImGui::SameLine();

        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }

        if (Confirmed && (mContentAction == EContentAction::Rename ? RenameContent() : DeleteContent())) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    /**
     * @brief 대상 항목의 이름을 바꿉니다. 스크립트는 파일 안의 클래스 이름도 바꿉니다.
     */
    bool Editor::FEditorUI::RenameContent() {
        const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};
        const Core::FString NewName{mContentActionName.data()};
        std::error_code Code{};
        const bool Directory{std::filesystem::is_directory(Absolute, Code)};

        if (mContentActionError = FindContentActionBlocker(Absolute); !mContentActionError.empty()) {
            return false;
        }

        if (NewName.empty() || NewName.find_first_of("\\/:*?\"<>|") != Core::FString::npos || NewName.back() == '.' || NewName.back() == ' ') {
            mContentActionError = "이름이 비어 있거나 쓸 수 없는 문자가 들어 있습니다.";

            return false;
        }

        // 스크립트는 클래스 이름과 파일 이름이 같아야 하므로 확장자를 떼고 식별자 규칙을 따릅니다.
        const std::filesystem::path NewFile{mContentActionScript ? std::filesystem::path{Core::FilePath::FromUtf8(NewName.c_str())}.replace_extension(".cs") : Directory ? std::filesystem::path{Core::FilePath::FromUtf8(NewName.c_str())} : std::filesystem::path{Core::FilePath::FromUtf8(NewName.c_str())} += mContentActionTarget.extension()};
        const std::filesystem::path Destination{Absolute.parent_path() / NewFile};
        const Core::FString ClassName{Core::FilePath::ToUtf8(NewFile.stem())};

        if (mContentActionScript && !std::regex_match(ClassName, std::regex{"[A-Za-z_][A-Za-z0-9_]*"})) {
            mContentActionError = "클래스 이름은 영문자나 _로 시작하고 영문자, 숫자, _만 쓸 수 있습니다.";

            return false;
        }

        if (Destination == Absolute) {
            return true;
        }

        // 대소문자만 바꾸는 경우는 같은 파일로 보이므로 존재 검사를 건너뜁니다.
        if (std::filesystem::exists(Destination, Code) && !std::filesystem::equivalent(Destination, Absolute, Code)) {
            mContentActionError = "같은 이름의 항목이 이미 있습니다.";

            return false;
        }

        std::filesystem::rename(Absolute, Destination, Code);

        if (Code) {
            mContentActionError = "이름을 바꾸지 못했습니다: " + Code.message();

            return false;
        }

        if (mContentActionScript) {
            std::ifstream Input{Destination, std::ios::binary};
            const std::string Text{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}};

            Input.close();

            const std::regex Declaration{"\\b(class|struct)(\\s+)" + Core::FilePath::ToUtf8(mContentActionTarget.stem()) + "\\b"};
            const std::string Renamed{std::regex_replace(Text, Declaration, "$1$2" + ClassName, std::regex_constants::format_first_only)};

            if (Renamed != Text) {
                std::ofstream{Destination, std::ios::binary} << Renamed;
            }

            mScriptBuilds.RefreshScriptFiles();
            mSelectedContentPath = Destination;
        } else {
            // 옛 경로의 캐시 객체는 바뀌지 않으므로 은퇴시켜, 나중에 같은 경로를 다시 쓸 때 새로 로드하게 합니다.
            mContext->GetProject()->GetAssets()->RetireAssets(Absolute);

            if (mMeshViewer != nullptr) {
                mMeshViewer->CloseRetired(Absolute);
            }

            if (mBlendSpaceEditor != nullptr) {
                mBlendSpaceEditor->CloseRetired(Absolute);
            }

            if (mAnimGraphEditor != nullptr) {
                mAnimGraphEditor->CloseRetired(Absolute);
            }

            mSelectedContentPath = mContentActionTarget.parent_path() / NewFile;
            mRefreshContent = true;

            if (mContentDirectory == mContentActionTarget) {
                mContentDirectory = mSelectedContentPath;
            }
        }

        mContext->AddLog("Renamed " + Core::FilePath::ToGenericUtf8(mContentActionTarget) + " to " + Core::FilePath::ToUtf8(NewFile));

        return true;
    }

    /**
     * @brief 대상 항목을 휴지통으로 옮깁니다.
     */
    bool Editor::FEditorUI::DeleteContent() {
        const std::filesystem::path Absolute{GetContentActionPath(mContentActionTarget)};

        if (mContentActionError = FindContentActionBlocker(Absolute); !mContentActionError.empty()) {
            return false;
        }

        // 실수로 지워도 되돌릴 수 있도록 휴지통으로 옮깁니다. 경로 목록은 두 개의 NUL로 끝나야 합니다.
        std::wstring From{Absolute.native()};
        From.push_back(L'\0');

        SHFILEOPSTRUCTW Operation{};
        Operation.hwnd = mPlatform->GetWindowHandle();
        Operation.wFunc = FO_DELETE;
        Operation.pFrom = From.c_str();
        Operation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;

        if (SHFileOperationW(&Operation) != 0 || Operation.fAnyOperationsAborted) {
            mContentActionError = "휴지통으로 옮기지 못했습니다. 다른 프로그램이 파일을 쓰고 있는지 확인하세요.";

            return false;
        }

        mSelectedContentPath.clear();

        if (mContentActionScript) {
            mScriptBuilds.RefreshScriptFiles();
        } else {
            // 지운 에셋을 참조하던 객체는 살려 두고, 같은 경로에 다시 임포트하면 새 객체를 로드하게 합니다.
            mContext->GetProject()->GetAssets()->RetireAssets(Absolute);

            if (mMeshViewer != nullptr) {
                mMeshViewer->CloseRetired(Absolute);
            }

            if (mBlendSpaceEditor != nullptr) {
                mBlendSpaceEditor->CloseRetired(Absolute);
            }

            if (mAnimGraphEditor != nullptr) {
                mAnimGraphEditor->CloseRetired(Absolute);
            }

            mRefreshContent = true;
        }

        mContext->AddLog("Moved " + Core::FilePath::ToGenericUtf8(mContentActionTarget) + " to the Recycle Bin");

        return true;
    }

    /**
     * @brief 콘텐츠 브라우저의 우클릭 메뉴 항목을 표시합니다.
     */
    void Editor::FEditorUI::DrawContentContextMenu() {
        const bool CanEdit{mContext->CanEditWorld()};

        // 스크립트 폴더에는 엔진 에셋을 만들 수 없으므로 에셋 항목을 감춥니다.
        if (!mBrowsingScripts) {
            if (ImGui::MenuItem("New Folder", nullptr, false, CanEdit)) {
                CreateContentFolder();
            }

            if (ImGui::MenuItem("New Material", nullptr, false, CanEdit)) {
                CreateMaterial();
            }

            if (ImGui::MenuItem("New Blend Space 1D", nullptr, false, CanEdit)) {
                CreateBlendSpace();
            }

            if (ImGui::MenuItem("New Anim Graph", nullptr, false, CanEdit)) {
                CreateAnimGraph();
            }
        }

        if (ImGui::MenuItem("New C# Script...")) {
            mNewScriptName.fill('\0');
            mNewScriptError.clear();
            mOpenNewScript = true;
        }

        if (!mSelectedContentPath.empty()) {
            ImGui::Separator();

            if (ImGui::MenuItem("Rename...", "F2", false, CanEdit)) {
                BeginContentAction(EContentAction::Rename);
            }

            if (ImGui::MenuItem("Delete", "Del", false, CanEdit)) {
                BeginContentAction(EContentAction::Delete);
            }
        }

        if (!mBrowsingScripts) {
            ImGui::Separator();

            if (ImGui::MenuItem("Import...", nullptr, false, CanEdit)) {
                ImportAsset();
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Show in Explorer")) {
            ShowContentInExplorer();
        }

        if (ImGui::MenuItem("Refresh")) {
            mRefreshContent = true;
        }
    }

    /**
     * @brief 현재 콘텐츠 폴더에 겹치지 않는 이름으로 새 폴더를 만듭니다.
     */
    void Editor::FEditorUI::CreateContentFolder() {
        const std::filesystem::path Root{mContext->GetProject()->GetAssets()->GetContentRoot()};
        std::filesystem::path Path{mContentDirectory / "NewFolder"};
        std::error_code Code{};

        for (Core::Uint32 Index{1}; std::filesystem::exists(Root / Path, Code) && !Code; ++Index) {
            Path = mContentDirectory / ("NewFolder_" + std::to_string(Index));
        }

        if (Code || !std::filesystem::create_directory(Root / Path, Code)) {
            mContext->AddLog("Cannot create the folder " + Core::FilePath::ToGenericUtf8(Path));

            return;
        }

        mRefreshContent = true;
        mSelectedContentPath = Path;
    }

    /**
     * @brief 선택한 항목이나 현재 폴더를 Windows 탐색기에서 보여 줍니다.
     */
    void Editor::FEditorUI::ShowContentInExplorer() const {
        std::filesystem::path Target{};

        if (mBrowsingScripts) {
            Target = !mSelectedContentPath.empty() ? mSelectedContentPath : mContext->GetProject()->GetRootPath() / "Scripts";
        } else {
            Target = mContext->GetProject()->GetAssets()->GetContentRoot() / (!mSelectedContentPath.empty() ? mSelectedContentPath : mContentDirectory);
        }

        std::error_code Code{};

        Target = std::filesystem::weakly_canonical(Target, Code).make_preferred();

        // 파일은 상위 폴더를 열고 그 파일을 선택하며, 폴더는 그 폴더를 엽니다.
        const std::wstring Arguments{std::filesystem::is_directory(Target, Code) ? L"\"" + Target.native() + L"\"" : L"/select,\"" + Target.native() + L"\""};

        ShellExecuteW(nullptr, L"open", L"explorer.exe", Arguments.c_str(), nullptr, SW_SHOWNORMAL);
    }

    /**
     * @brief 새 C# 스크립트의 클래스 이름을 묻는 창을 표시합니다.
     */
    void Editor::FEditorUI::DrawNewScriptPopup() {
        constexpr const char* PopupName{"New C# Script"};

        if (mOpenNewScript) {
            ImGui::OpenPopup(PopupName);

            mOpenNewScript = false;
        }

        if (!ImGui::BeginPopupModal(PopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            return;
        }

        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        const bool Submitted{ImGui::InputTextWithHint("Class Name", "PlayerController", mNewScriptName.data(), mNewScriptName.size(), ImGuiInputTextFlags_EnterReturnsTrue)};

        ImGui::TextDisabled("Scripts/<Class Name>.cs 파일을 만들고 Visual Studio에서 엽니다.");

        if (!mNewScriptError.empty()) {
            ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", mNewScriptError.c_str());
        }

        if (ImGui::Button("Create") || Submitted) {
            if (mScriptBuilds.CreateScript(mNewScriptName.data(), mNewScriptError)) {
                mBrowsingScripts = true;

                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    /**
     * @brief 선택한 텍스처의 미리보기 창을 그립니다.
     */
    void Editor::FEditorUI::DrawTexturePreview() {
        if (mPreviewTexture == nullptr || !mPreviewTexture->HasResources()) {
            return;
        }

        bool Open{true};
        const float UIScale{mUIScale.GetScale()};

        ImGui::SetNextWindowSize({640.0f * UIScale, 560.0f * UIScale}, ImGuiCond_FirstUseEver);

        if (mFocusTexturePreview) {
            ImGui::SetNextWindowFocus();
            ImGui::SetNextWindowCollapsed(false);

            mFocusTexturePreview = false;
        }

        if (ImGui::Begin("Texture Preview", &Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(mPreviewTexturePath).c_str());
            ImGui::Text("Size: %u x %u", mPreviewTexture->GetSizeX(), mPreviewTexture->GetSizeY());
            ImGui::Separator();

            const ImVec2 Available{ImGui::GetContentRegionAvail()};

            if (Available.x > 0.0f && Available.y > 0.0f) {
                const float SizeX{static_cast<float>(mPreviewTexture->GetSizeX())};
                const float SizeY{static_cast<float>(mPreviewTexture->GetSizeY())};
                const float Scale{std::min(Available.x / SizeX, Available.y / SizeY)};
                const ImVec2 ImageSize{SizeX * Scale, SizeY * Scale};
                const ImVec2 Origin{ImGui::GetCursorPos()};

                ImGui::SetCursorPos({Origin.x + (Available.x - ImageSize.x) * 0.5f, Origin.y + (Available.y - ImageSize.y) * 0.5f});
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(mPreviewTexture->GetShaderResourceView()))}, ImageSize);
            }
        }

        ImGui::End();

        if (!Open) {
            mPreviewTexture = nullptr;
            mPreviewTexturePath.clear();
        }
    }

    /**
     * @brief 현재 Content 폴더와 에셋 종류 목록을 다시 읽습니다.
     */
    void Editor::FEditorUI::RefreshContent() {
        mAssetPaths.clear();
        mAssetTypes.clear();
        mContentFolders.clear();
        mContentFolders[{}] = {};
        mContentError.clear();

        if (mAssetCatalog == nullptr) {
            return;
        }

        mAssetCatalog->Refresh(mContentError);

        for (const auto& Directory : mAssetCatalog->GetDirectories()) {
            mContentFolders[Directory.parent_path()].push_back(Directory);
            mContentFolders.try_emplace(Directory);
        }

        for (const auto& Entry : mAssetCatalog->GetEntries()) {
            mAssetPaths.push_back(Entry.GetPath());

            if (Entry.GetType().has_value()) {
                mAssetTypes.emplace(Entry.GetPath(), *Entry.GetType());
            }
        }

        for (auto& Entry : mContentFolders) {
            std::sort(Entry.second.begin(), Entry.second.end());
            Entry.second.erase(std::unique(Entry.second.begin(), Entry.second.end()), Entry.second.end());
        }

        while (!mContentDirectory.empty() && !mContentFolders.contains(mContentDirectory)) {
            mContentDirectory = mContentDirectory.parent_path();
        }

        mThumbnails->Invalidate();
        mPlaceActorsPanel.InvalidateIcons();
        mRefreshContent = false;
        mRevealContentDirectory = true;
    }

    /**
     * @brief 콘텐츠 브라우저의 현재 폴더와 선택을 변경합니다.
     */
    void Editor::FEditorUI::NavigateContent(const std::filesystem::path& Directory) {
        if (mContentFolders.contains(Directory)) {
            mBrowsingScripts = false;
            mContentDirectory = Directory;
            mSelectedContentPath.clear();
            mRevealContentDirectory = true;
        }
    }

    /**
     * @brief Content 하위 폴더 트리를 재귀적으로 표시합니다.
     */
    void Editor::FEditorUI::DrawContentTree(const std::filesystem::path& Directory) {
        const auto Entry{mContentFolders.find(Directory)};

        if (Entry == mContentFolders.end()) {
            return;
        }

        const Core::FString Label{Directory.empty() ? "Content" : Core::FilePath::ToUtf8(Directory.filename())};
        const Core::FString Id{Core::FilePath::ToUtf8(Directory)};
        ImGuiTreeNodeFlags Flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

        if (Directory == mContentDirectory && !mBrowsingScripts) {
            Flags |= ImGuiTreeNodeFlags_Selected;
        }

        if (Entry->second.empty()) {
            Flags |= ImGuiTreeNodeFlags_Leaf;
        }

        if (Directory.empty()) {
            Flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        const auto Relative{mContentDirectory.lexically_relative(Directory)};

        if (mRevealContentDirectory && (Directory.empty() || Directory == mContentDirectory || (!Relative.empty() && *Relative.begin() != L".."))) {
            ImGui::SetNextItemOpen(true);
        }

        ImGui::PushID(Id.c_str());

        const bool Open{ImGui::TreeNodeEx(Label.c_str(), Flags)};
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        const ImVec2 Maximum{ImGui::GetItemRectMax()};

        RegisterContentDropTarget(Directory, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});

        if (ImGui::IsItemClicked()) {
            NavigateContent(Directory);
        }

        if (Open) {
            for (const auto& Child : Entry->second) {
                DrawContentTree(Child);
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    /**
     * @brief 현재 콘텐츠 폴더의 상위 경로 탐색 항목을 표시합니다.
     */
    void Editor::FEditorUI::DrawContentBreadcrumbs() {
        ImGui::PushID("Content Path");

        if (mBrowsingScripts) {
            if (ImGui::SmallButton("Content")) {
                NavigateContent({});
            }

            ImGui::SameLine();
            ImGui::TextUnformatted("|");
            ImGui::SameLine();
            ImGui::TextUnformatted("Scripts");
            ImGui::PopID();

            return;
        }

        ImGui::BeginDisabled(mContentDirectory.empty());

        if (ImGui::SmallButton("Up")) {
            NavigateContent(mContentDirectory.parent_path());
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (ImGui::SmallButton("Content")) {
            NavigateContent({});
        }

        const std::filesystem::path Current{mContentDirectory};
        std::filesystem::path Path{};

        for (const auto& Part : Current) {
            Path /= Part;

            const Core::FString Label{Core::FilePath::ToUtf8(Part)};
            const float Right{ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x};

            if (Right - ImGui::GetItemRectMax().x > ImGui::CalcTextSize(Label.c_str()).x + 48.0f) {
                ImGui::SameLine();
            }

            ImGui::TextUnformatted("/");
            ImGui::SameLine();

            const Core::FString Id{Core::FilePath::ToUtf8(Path)};

            ImGui::PushID(Id.c_str());

            if (ImGui::SmallButton(Label.c_str())) {
                NavigateContent(Path);
            }

            ImGui::PopID();
        }

        ImGui::PopID();
    }

    /**
     * @brief 현재 폴더의 하위 폴더·에셋 항목과 드래그 원본을 표시합니다.
     */
    void Editor::FEditorUI::DrawContentEntries() {
        const ImGuiTextFilter Search{mContentSearch.data()};
        const bool Recursive{mContentFilter.has_value() || mContentSearch[0] != '\0'};
        Core::TArray<std::filesystem::path> Folders{};
        Core::TArray<std::filesystem::path> Assets{};
        const auto Folder{mContentFolders.find(mContentDirectory)};

        if (!Recursive && Folder != mContentFolders.end() && !mBrowsingScripts) {
            Folders = Folder->second;
        }

        // 스크립트 폴더는 하위 폴더 없이 모든 C# 파일을 한 번에 보여 줍니다.
        if (mBrowsingScripts) {
            for (const auto& Path : mScriptBuilds.GetScriptFiles()) {
                if (Search.PassFilter(Core::FilePath::ToGenericUtf8(Path.filename()).c_str())) {
                    Assets.push_back(Path);
                }
            }
        }

        for (const auto& Path : mAssetPaths) {
            if (mBrowsingScripts) {
                break;
            }

            const auto Type{mAssetTypes.find(Path)};
            const auto Relative{Path.lexically_relative(mContentDirectory)};
            const bool InFolder{Recursive ? mContentDirectory.empty() || (!Relative.empty() && *Relative.begin() != L"..") : Path.parent_path() == mContentDirectory};

            if (!InFolder || (mContentFilter.has_value() && (Type == mAssetTypes.end() || Type->second != *mContentFilter)) || !Search.PassFilter(Core::FilePath::ToGenericUtf8(Path).c_str())) {
                continue;
            }

            Assets.push_back(Path);
        }

        const int Count{static_cast<int>(Folders.size() + Assets.size())};

        if (Count == 0) {
            ImGui::TextDisabled(mBrowsingScripts ? "No C# scripts match this search." : "No assets match this folder or search.");

            return;
        }

        const float Scale{mUIScale.GetScale()};
        const float Padding{4.0f * Scale};
        const bool Compact{ImGui::GetContentRegionAvail().y < 164.0f * Scale};
        const float TileWidth{(Compact ? 180.0f : 120.0f) * Scale};
        const float ImageSize{(Compact ? 48.0f : 96.0f) * Scale};
        const float ImageOffset{(Compact ? 4.0f : 10.0f) * Scale};
        const float TextX{Compact ? ImageOffset + ImageSize + 8.0f * Scale : Padding};
        const float TypeY{Compact ? Padding : ImageSize + Padding * 2.0f};
        const float NameY{TypeY + ImGui::GetTextLineHeightWithSpacing()};
        const float NameHeight{ImGui::GetTextLineHeight() * 2.0f + ImGui::GetStyle().ItemSpacing.y};
        const float TileHeight{std::max(Padding + ImageSize, NameY + NameHeight) + Padding * 2.0f};
        const int Columns{std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / (TileWidth + 8.0f * Scale)), 1, 12)};
        bool Navigate{};
        std::filesystem::path Requested{};

        if (ImGui::BeginTable("ContentGrid", Columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX)) {
            for (int Column{}; Column < Columns; ++Column) {
                ImGui::TableSetupColumn("##AssetColumn", ImGuiTableColumnFlags_WidthFixed, TileWidth);
            }

            ImGuiListClipper Clipper{};
            Clipper.Begin((Count + Columns - 1) / Columns, TileHeight);

            while (Clipper.Step()) {
                for (int Row{Clipper.DisplayStart}; Row < Clipper.DisplayEnd; ++Row) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, TileHeight);

                    for (int Column{}; Column < Columns; ++Column) {
                        const int Index{Row * Columns + Column};

                        if (Index >= Count) {
                            break;
                        }

                        ImGui::TableSetColumnIndex(Column);

                        const bool Directory{static_cast<std::size_t>(Index) < Folders.size()};
                        const auto& Path{Directory ? Folders[Index] : Assets[Index - Folders.size()]};
                        const auto Type{mAssetTypes.find(Path)};
                        const bool HasThumbnail{!Directory && Type != mAssetTypes.end() && (Type->second == Asset::EAssetType::Mesh || Type->second == Asset::EAssetType::SkeletalMesh || Type->second == Asset::EAssetType::Skeleton || Type->second == Asset::EAssetType::Material || Type->second == Asset::EAssetType::Texture)};
                        void* Thumbnail{HasThumbnail ? mThumbnails->GetThumbnail(Path, Type->second) : nullptr};
                        const bool ScriptFile{mBrowsingScripts && !Directory};
                        const char* Kind{ScriptFile ? "C# Script" : Directory ? "Folder" : Type == mAssetTypes.end() ? "File" : Type->second == Asset::EAssetType::Mesh ? "Mesh" : Type->second == Asset::EAssetType::SkeletalMesh ? "Skeletal Mesh" : Type->second == Asset::EAssetType::Skeleton ? "Skeleton" : Type->second == Asset::EAssetType::Material ? "Material" : Type->second == Asset::EAssetType::Texture ? "Texture" : Type->second == Asset::EAssetType::Sequence ? "Sequence" : Type->second == Asset::EAssetType::AnimSequence ? "Animation Sequence" : Type->second == Asset::EAssetType::BlendSpace ? "Blend Space" : Type->second == Asset::EAssetType::AnimGraph ? "Anim Graph" : "Scene"};
                        const Core::FString Actual{Core::FilePath::ToGenericUtf8(Path)};
                        const Core::FString Name{Core::FilePath::ToUtf8(Directory ? Path.filename() : Path.stem())};
                        const FContentIcon& Icon{GetContentIcon(Kind)};
                        const auto CachedIcon{mContentIcons.find(Icon.mKind)};
                        ID3D11ShaderResourceView* Resource{CachedIcon != mContentIcons.end() ? CachedIcon->second.Get() : nullptr};

                        ImGui::PushID(Actual.c_str());

                        const ImVec2 Origin{ImGui::GetCursorScreenPos()};

                        if (ImGui::Selectable("##AssetTile", mSelectedContentPath == Path, ImGuiSelectableFlags_AllowDoubleClick, {TileWidth - Padding, TileHeight - Padding * 2.0f})) {
                            mSelectedContentPath = Path;

                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                if (Directory) {
                                    Requested = Path;
                                    Navigate = true;
                                } else if (ScriptFile) {
                                    mScriptBuilds.OpenScriptFile(Path);
                                } else if (Type != mAssetTypes.end() && mContext->CanEditWorld()) {
                                    OpenContentAsset(Path, Type->second);
                                }
                            }
                        }

                        // 우클릭 메뉴가 이 항목을 대상으로 하도록 먼저 선택합니다.
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                            mSelectedContentPath = Path;
                        }

                        const bool Hovered{ImGui::IsItemHovered()};

                        if (Directory) {
                            const ImVec2 Minimum{ImGui::GetItemRectMin()};
                            const ImVec2 Maximum{ImGui::GetItemRectMax()};

                            RegisterContentDropTarget(Path, {Minimum.x, Minimum.y, Maximum.x, Maximum.y});
                        }

                        // 애니메이션 시퀀스는 썸네일이 없지만 Details의 Animation 필드에 놓을 수 있도록 끌기를 허용합니다.
                        const bool AnimSequence{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::AnimSequence};
                        const bool BlendSpace{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::BlendSpace};
                        const bool AnimGraph{!Directory && Type != mAssetTypes.end() && Type->second == Asset::EAssetType::AnimGraph};

                        if ((HasThumbnail || AnimSequence || BlendSpace || AnimGraph) && mContext->CanEditWorld() && ImGui::BeginDragDropSource()) {
                            const char* Payload{AnimSequence ? Editor::EditorAssetDragDrop::AnimSequencePayload : BlendSpace ? Editor::EditorAssetDragDrop::BlendSpacePayload : AnimGraph ? Editor::EditorAssetDragDrop::AnimGraphPayload : Type->second == Asset::EAssetType::Mesh || Type->second == Asset::EAssetType::SkeletalMesh ? Editor::EditorAssetDragDrop::MeshPayload : Type->second == Asset::EAssetType::Skeleton ? Editor::EditorAssetDragDrop::SkeletonPayload : Type->second == Asset::EAssetType::Material ? Editor::EditorAssetDragDrop::MaterialPayload : Editor::EditorAssetDragDrop::TexturePayload};

                            ImGui::SetDragDropPayload(Payload, Actual.c_str(), Actual.size() + 1);

                            DrawContentImage(64.0f * Scale, Thumbnail, Resource, Icon);
                            ImGui::TextUnformatted(Name.c_str());
                            ImGui::EndDragDropSource();
                        }

                        if (Hovered) {
                            ImGui::BeginTooltip();
                            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(Path).c_str());
                            ImGui::TextDisabled("%s", Icon.mTooltip);
                            ImGui::EndTooltip();
                        }

                        const ImVec2 Next{ImGui::GetCursorScreenPos()};

                        ImGui::SetCursorScreenPos({Origin.x + ImageOffset, Origin.y + Padding});
                        DrawContentImage(ImageSize, Thumbnail, Resource, Icon);
                        ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + TypeY});
                        ImGui::TextDisabled("%s", Kind);
                        ImGui::SetCursorScreenPos({Origin.x + TextX, Origin.y + NameY});
                        ImGui::PushClipRect({Origin.x + TextX, Origin.y + NameY}, {Origin.x + TileWidth - Padding, Origin.y + NameY + NameHeight}, true);
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + TileWidth - TextX - 10.0f * Scale);
                        ImGui::TextWrapped("%s", Name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::PopClipRect();
                        ImGui::SetCursorScreenPos(Next);
                        ImGui::Dummy({0.0f, 0.0f});
                        ImGui::PopID();
                    }
                }
            }

            ImGui::EndTable();
        }

        if (Navigate) {
            NavigateContent(Requested);
        }
    }

    /**
     * @brief 종류에 맞게 선택 에셋을 열거나 액터 배치에 사용합니다.
     */
    void Editor::FEditorUI::OpenContentAsset(const std::filesystem::path& Path, Asset::EAssetType Type) {
        if (Type == Asset::EAssetType::Scene) {
            if (ConfirmSceneChange()) {
                mContext->LoadScene(Path);
            }
        } else if (Type == Asset::EAssetType::Mesh || Type == Asset::EAssetType::SkeletalMesh) {
            Core::FString Error{};

            if (mMeshViewer != nullptr && !mMeshViewer->Open(Path, Error)) {
                mContext->AddLog(Error);
            }
        } else if (Type == Asset::EAssetType::Material) {
            if (!ConfirmMaterialChange()) {
                return;
            }

            Asset::UMaterial* Material{mContext->GetProject()->GetAssets()->LoadMaterial(Path, mMaterialError)};

            if (Material != nullptr) {
                mEditingMaterial = Material;
                mMaterialPath = Path;
                mMaterialDirty = false;
            } else {
                mContext->AddLog(mMaterialError);
            }
        } else if (Type == Asset::EAssetType::Texture) {
            Core::FString Error{};
            Asset::UTexture2D* Texture{mContext->GetProject()->GetAssets()->LoadTexture(Path, Error)};

            if (Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(mContext->GetRenderer()->GetNativeDevice()))) {
                mPreviewTexture = Texture;
                mPreviewTexturePath = Path;
                mFocusTexturePreview = true;
                mContext->AddLog("Loaded " + Core::FilePath::ToUtf8(Path));
            } else {
                mContext->AddLog(Error.empty() ? "Cannot preview the texture" : Error);
            }
        } else if (Type == Asset::EAssetType::AnimGraph) {
            Core::FString Error{};

            if (mAnimGraphEditor != nullptr && !mAnimGraphEditor->Open(Path, Error)) {
                mContext->AddLog(Error.empty() ? "Cannot open the anim graph" : Error);
            }
        } else if (Type == Asset::EAssetType::BlendSpace) {
            Core::FString Error{};

            if (mBlendSpaceEditor != nullptr && !mBlendSpaceEditor->Open(Path, Error)) {
                mContext->AddLog(Error.empty() ? "Cannot open the blend space" : Error);
            }
        } else if (Type == Asset::EAssetType::AnimSequence) {
            // 재생 기능이 생기기 전까지는 임포트 결과를 확인할 수 있도록 시간 정보와 트랙 요약을 로그로 출력합니다.
            Core::FString Error{};
            const Asset::UAnimSequence* Sequence{mContext->GetProject()->GetAssets()->LoadAnimSequence(Path, Error)};

            if (Sequence == nullptr) {
                mContext->AddLog(Error.empty() ? "Cannot load the animation sequence" : Error);

                return;
            }

            const Asset::FAnimDataModel& Model{Sequence->GetDataModel()};
            mAnimSequencePath = Path;
            mAnimSequenceOpen = true;
            mAnimSequenceRootMotion = Sequence->IsRootMotionEnabled();
            mAnimSequenceForceRootLock = Sequence->IsRootLockForced();
            mAnimSequenceRootLock = static_cast<int>(Sequence->GetRootMotionRootLock());
            strncpy_s(mAnimSequenceRootBone.data(), mAnimSequenceRootBone.size(), Sequence->GetRootMotionBone().c_str(), _TRUNCATE);
            std::size_t AnimatedChannels{};
            std::size_t ConstantChannels{};

            for (const Asset::FBoneAnimationTrack& Track : Model.GetBoneAnimationTracks()) {
                for (const std::size_t Count : {Track.mInternalTrack.mPosKeys.size(), Track.mInternalTrack.mRotKeys.size(), Track.mInternalTrack.mScaleKeys.size()}) {
                    (Count == 1 ? ConstantChannels : AnimatedChannels) += 1;
                }
            }

            mContext->AddLog(std::format("Animation '{}': {} frames at {:.3f} fps ({:.3f} s), {} tracks, {} animated / {} constant channels", Core::FilePath::ToUtf8(Path.stem()), Model.GetNumberOfFrames(), Model.GetFrameRate().AsDecimal(), Model.GetPlayLength(), Model.GetBoneAnimationTracks().size(), AnimatedChannels, ConstantChannels));
        }
    }

    void FEditorUI::DrawAnimSequenceSettings() {
        if (!mAnimSequenceOpen || mContext->GetProject() == nullptr) {
            return;
        }

        ImGui::SetNextWindowSize({480.0f, 280.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Animation Sequence", &mAnimSequenceOpen)) {
            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(mAnimSequencePath).c_str());
            ImGui::Separator();
            ImGui::BeginDisabled(!mContext->CanEditWorld());
            ImGui::Checkbox("Enable Root Motion", &mAnimSequenceRootMotion);
            ImGui::Combo("Root Motion Root Lock", &mAnimSequenceRootLock, "Ref Pose\0Anim First Frame\0Zero\0");
            ImGui::Checkbox("Force Root Lock", &mAnimSequenceForceRootLock);
            ImGui::InputText("Root Bone", mAnimSequenceRootBone.data(), mAnimSequenceRootBone.size());
            ImGui::TextDisabled("Empty: use the skeleton's single root bone.");

            if (ImGui::Button("Save")) {
                Core::FString Error{};
                Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
                Asset::UAnimSequence* Sequence{Assets->LoadAnimSequence(mAnimSequencePath, Error)};

                if (Sequence != nullptr) {
                    const bool PreviousEnabled{Sequence->IsRootMotionEnabled()};
                    const bool PreviousForced{Sequence->IsRootLockForced()};
                    const Asset::ERootMotionRootLock PreviousLock{Sequence->GetRootMotionRootLock()};
                    const Core::FString PreviousBone{Sequence->GetRootMotionBone()};

                    Sequence->SetRootMotionEnabled(mAnimSequenceRootMotion);
                    Sequence->SetForceRootLock(mAnimSequenceForceRootLock);
                    Sequence->SetRootMotionRootLock(static_cast<Asset::ERootMotionRootLock>(mAnimSequenceRootLock));
                    Sequence->SetRootMotionBone(mAnimSequenceRootBone.data());

                    if (!Assets->SaveAnimSequence(mAnimSequencePath, *Sequence, Error)) {
                        Sequence->SetRootMotionEnabled(PreviousEnabled);
                        Sequence->SetForceRootLock(PreviousForced);
                        Sequence->SetRootMotionRootLock(PreviousLock);
                        Sequence->SetRootMotionBone(PreviousBone);
                    } else {
                        mContext->AddLog("Saved animation root motion settings");
                    }
                }

                if (!Error.empty()) {
                    mContext->AddLog(Error);
                }
            }

            ImGui::EndDisabled();
        }

        ImGui::End();
    }

    /**
     * @brief 기본 설정으로 새 머티리얼 에셋을 만듭니다.
     */
    void Editor::FEditorUI::CreateMaterial() {
        if (!ConfirmMaterialChange()) {
            return;
        }

        Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};
        std::filesystem::path Path{mContentDirectory / "Material.masset"};
        std::error_code Code{};

        for (Core::Uint32 Index{1}; std::filesystem::exists(Assets->GetContentRoot() / Path, Code) && !Code; ++Index) {
            Path = mContentDirectory / ("Material_" + std::to_string(Index) + ".masset");
        }

        auto Material{CoreUObject::NewObject<Asset::UMaterial>()};
        Material->SetName(Core::FilePath::ToUtf8(Path.stem()));

        if (Code || !Assets->SaveMaterial(Path, *Material, mMaterialError)) {
            mContext->AddLog(mMaterialError.empty() ? "Cannot create the material" : mMaterialError);

            return;
        }

        mRefreshContent = true;
        mSelectedContentPath = Path;

        OpenContentAsset(Path, Asset::EAssetType::Material);
    }

    void Editor::FEditorUI::CreateAnimGraph() {
        Core::FString Error{};
        std::filesystem::path Path{};

        if (mContext->GetProject()->GetAssets()->CreateAnimGraph(mContentDirectory, "AnimGraph", Path, Error) == nullptr) {
            mContext->AddLog(Error.empty() ? "Cannot create the anim graph" : Error);

            return;
        }

        mRefreshContent = true;
        mSelectedContentPath = Path;

        OpenContentAsset(Path, Asset::EAssetType::AnimGraph);
    }

    void Editor::FEditorUI::CreateBlendSpace() {
        Core::FString Error{};
        std::filesystem::path Path{};

        if (mContext->GetProject()->GetAssets()->CreateBlendSpace1D(mContentDirectory, "BlendSpace", Path, Error) == nullptr) {
            mContext->AddLog(Error.empty() ? "Cannot create the blend space" : Error);

            return;
        }

        mRefreshContent = true;
        mSelectedContentPath = Path;

        OpenContentAsset(Path, Asset::EAssetType::BlendSpace);
    }

    /**
     * @brief 편집 사본을 프로젝트 머티리얼에 저장합니다.
     */
    bool Editor::FEditorUI::SaveMaterial() {
        if (mEditingMaterial == nullptr || !mContext->GetProject()->GetAssets()->SaveMaterial(mMaterialPath, *mEditingMaterial, mMaterialError)) {
            return false;
        }

        mMaterialDirty = false;
        mRefreshContent = true;
        mContext->AddLog("Saved " + Core::FilePath::ToUtf8(mMaterialPath));

        return true;
    }

    /**
     * @brief 미저장 머티리얼을 닫기 전 사용자 선택을 처리합니다.
     */
    bool Editor::FEditorUI::ConfirmMaterialChange() {
        if (!mMaterialDirty || mEditingMaterial == nullptr) {
            return true;
        }

        const Core::Int32 Choice{MessageBoxW(mPlatform->GetWindowHandle(), L"Save changes to the current material?", L"Material", MB_YESNOCANCEL | MB_ICONQUESTION)};

        if (Choice == IDCANCEL) {
            return false;
        }

        if (Choice == IDYES) {
            return SaveMaterial();
        }

        if (!mContext->GetProject()->GetAssets()->ReloadMaterial(mMaterialPath, mMaterialError)) {
            return false;
        }

        mMaterialDirty = false;

        return true;
    }

    /**
     * @brief 머티리얼 편집 사본의 텍스처 에셋을 검증하여 교체합니다.
     */
    bool Editor::FEditorUI::SetMaterialTexture(const Core::FString& Path) {
        Asset::FAssetManager* Assets{mContext->GetProject()->GetAssets()};

        if (!Path.empty() && Assets->LoadTexture(Core::FilePath::FromUtf8(Path), mMaterialError) == nullptr) {
            return false;
        }

        if (!mEditingMaterial->SetBaseColorTexturePath(Path) || !mEditingMaterial->ResolveTexture(*Assets, mMaterialError)) {
            if (mMaterialError.empty()) {
                mMaterialError = "Invalid texture path";
            }

            return false;
        }

        mMaterialError.clear();
        mMaterialDirty = true;

        return true;
    }

    /**
     * @brief 머티리얼 편집 사본의 셰이더 경로를 검증하여 교체합니다.
     */
    bool Editor::FEditorUI::SetMaterialShader(const Core::FString& Path) {
        auto Candidate{CoreUObject::NewObject<Asset::UMaterial>()};
        Candidate->SetTwoSided(mEditingMaterial->IsTwoSided());

        Renderer::FRenderer* Renderer{dynamic_cast<Renderer::FRenderer*>(mContext->GetRenderer())};

        if (!Candidate->SetShaderPath(Path) || Renderer == nullptr || Renderer->GetMaterialPipeline(*Candidate) == nullptr) {
            mMaterialError = "Cannot use this material shader. Check the shader diagnostics in the log.";

            return false;
        }

        mEditingMaterial->SetShaderPath(Path);
        mMaterialError.clear();
        mMaterialDirty = true;

        return true;
    }

    /**
     * @brief 편집 중인 머티리얼의 프로퍼티와 저장 상태를 표시합니다.
     */
    void Editor::FEditorUI::DrawMaterialEditor() {
        if (mEditingMaterial == nullptr) {
            return;
        }

        bool Open{true};

        ImGui::SetNextWindowSize({560.0f, 700.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin(mMaterialDirty ? "Material Editor*###MaterialEditor" : "Material Editor###MaterialEditor", &Open)) {
            ImGui::TextUnformatted(Core::FilePath::ToGenericUtf8(mMaterialPath).c_str());

            if (void* Preview{mThumbnails->GetMaterialPreview(mEditingMaterial)}) {
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Preview))}, {192.0f, 192.0f});
            }

            ImGui::BeginDisabled(!mContext->CanEditWorld());

            if (ImGui::Button("Save")) {
                SaveMaterial();
            }

            ImGui::SameLine();

            if (ImGui::Button("Reload") && ConfirmMaterialChange()) {
                mContext->GetProject()->GetAssets()->ReloadMaterial(mMaterialPath, mMaterialError);
            }

            ImGui::Separator();

            if (ImGui::BeginCombo("Shader", mEditingMaterial->GetShaderPath().c_str())) {
                Core::TArray<Core::FString> Shaders{"Shaders/Lit.hlsl", "Shaders/Unlit.hlsl"};

                for (const auto& Path : mAssetPaths) {
                    const Core::FString Name{Core::FilePath::ToUtf8(Path)};

                    if (Path.extension() == L".hlsl" && !Name.ends_with(".vs.hlsl") && Path.filename() != L"Line.hlsl" && Path.filename() != L"Gizmo.hlsl" && Path.filename() != L"Text.hlsl" && Path.filename() != L"DeferredLighting.hlsl" && std::find(Shaders.begin(), Shaders.end(), Name) == Shaders.end()) {
                        Shaders.push_back(Name);
                    }
                }

                for (const Core::FString& Shader : Shaders) {
                    if (ImGui::Selectable(Shader.c_str(), mEditingMaterial->GetShaderPath() == Shader)) {
                        SetMaterialShader(Shader);
                    }
                }

                ImGui::EndCombo();
            }

            std::array<char, 512> ShaderPath{};
            const Core::FString& CurrentShader{mEditingMaterial->GetShaderPath()};

            std::copy_n(CurrentShader.begin(), std::min(CurrentShader.size(), ShaderPath.size() - 1), ShaderPath.begin());

            if (ImGui::InputText("Shader Path", ShaderPath.data(), ShaderPath.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
                SetMaterialShader(ShaderPath.data());
            }

            auto Color{mEditingMaterial->GetBaseColor().ToArray()};

            if (ImGui::ColorEdit4("Base Color", Color.data()) && mEditingMaterial->SetBaseColor({Color[0], Color[1], Color[2], Color[3]})) {
                mMaterialDirty = true;
            }

            bool TwoSided{mEditingMaterial->IsTwoSided()};

            if (ImGui::Checkbox("Two Sided", &TwoSided)) {
                mEditingMaterial->SetTwoSided(TwoSided);
                mMaterialDirty = true;
            }

            Core::FString TexturePath{mEditingMaterial->GetBaseColorTexturePath()};

            if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Base Color Texture", TexturePath, Asset::EAssetType::Texture)) {
                SetMaterialTexture(TexturePath);
            }

            Core::FString NormalPath{mEditingMaterial->GetNormalMapTexturePath()};

            if (mAssetPicker.Draw(*mAssetCatalog, *mThumbnails, "Normal Map", NormalPath, Asset::EAssetType::Texture)) {
                const Core::FString Previous{mEditingMaterial->GetNormalMapTexturePath()};

                if (mEditingMaterial->SetNormalMapTexturePath(NormalPath) && mEditingMaterial->ResolveTexture(*mContext->GetProject()->GetAssets(), mMaterialError)) {
                    mMaterialDirty = true;
                } else {
                    mEditingMaterial->SetNormalMapTexturePath(Previous);

                    Core::FString RestoreError{};

                    mEditingMaterial->ResolveTexture(*mContext->GetProject()->GetAssets(), RestoreError);
                }
            }

            float Specular{mEditingMaterial->GetSpecularStrength()};
            float Shininess{mEditingMaterial->GetShininess()};
            float NormalStrength{mEditingMaterial->GetNormalStrength()};

            if (ImGui::SliderFloat("Specular Strength", &Specular, 0.0f, 1.0f) && mEditingMaterial->SetSpecularStrength(Specular)) {
                mMaterialDirty = true;
            }

            if (ImGui::SliderFloat("Shininess", &Shininess, 1.0f, 1024.0f, "%.1f", ImGuiSliderFlags_Logarithmic) && mEditingMaterial->SetShininess(Shininess)) {
                mMaterialDirty = true;
            }

            if (ImGui::SliderFloat("Normal Strength", &NormalStrength, 0.0f, 8.0f) && mEditingMaterial->SetNormalStrength(NormalStrength)) {
                mMaterialDirty = true;
            }

            ImGui::EndDisabled();

            if (!mMaterialError.empty()) {
                ImGui::TextWrapped("%s", mMaterialError.c_str());
            }
        }

        ImGui::End();

        if (!Open && ConfirmMaterialChange()) {
            mEditingMaterial = nullptr;
            mMaterialPath.clear();
            mMaterialError.clear();
        }
    }

}
