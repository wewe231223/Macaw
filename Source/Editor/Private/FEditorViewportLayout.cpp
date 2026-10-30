/**
 * @file FEditorViewportLayout.cpp
 * @brief 활성 뷰와 입력 캡처를 유지하며 단일·4분할 뷰를 배치합니다.
 * @ingroup Editor
 */

#include "FEditorViewportLayout.h"
#include "EditorStyle.h"
#include "Editor/FEditorContext.h"
#include "Editor/FEditorViewport.h"
#include "Editor/FPlayInEditorSession.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace Editor {

    Editor::FEditorViewportLayout::FEditorViewportLayout(Editor::FEditorContext* Context, Editor::FEditorViewport* Viewport, Editor::FPlayInEditorSession* PlaySession)
		: mContext{Context},
		  mViewport{Viewport},
		  mPlaySession{PlaySession} {
    }

    Editor::FEditorViewportLayout::~FEditorViewportLayout() {
        Shutdown();
    }

    bool Editor::FEditorViewportLayout::Initialize(RenderCore::IRendererModule* Renderer) {
        if (Renderer == nullptr || mContext == nullptr || mViewport == nullptr || mPlaySession == nullptr || mOrthographicViewports[0] != nullptr) {
            return false;
        }

        const std::array<Editor::EEditorViewportType, 3> Types{Editor::EEditorViewportType::Top, Editor::EEditorViewportType::Front, Editor::EEditorViewportType::Right};

        for (Core::Uint32 Index{}; Index < mOrthographicViewports.size(); ++Index) {
            mOrthographicViewports[Index] = std::make_unique<Editor::FEditorViewport>(mContext, "Orthographic" + std::to_string(Index));
            mOrthographicViewports[Index]->GetClient()->SetViewType(Types[Index]);

            if (!mOrthographicViewports[Index]->Initialize(Renderer)) {
                Shutdown();
                return false;
            }
        }

        if (ImGui::GetCurrentContext() != nullptr) {
            ImGuiSettingsHandler Handler{};
            Handler.TypeName = "MacawViewport";
            Handler.TypeHash = ImHashStr(Handler.TypeName);
            Handler.UserData = this;
            Handler.ReadOpenFn = ReadSettingsOpen;
            Handler.ReadLineFn = ReadSettingsLine;
            Handler.WriteAllFn = WriteSettings;
            ImGui::AddSettingsHandler(&Handler);
        }

        return true;
    }

    void Editor::FEditorViewportLayout::Shutdown() {
        if (ImGui::GetCurrentContext() != nullptr) {
            ImGuiSettingsHandler* Handler{ImGui::FindSettingsHandler("MacawViewport")};

            if (Handler != nullptr && Handler->UserData == this) {
                ImGui::RemoveSettingsHandler("MacawViewport");
            }
        }

        for (auto& Viewport : mOrthographicViewports) {
            Viewport.reset();
        }

        mVisible.fill(false);
        mActiveViewport = 0;
        mSingleViewport = 0;
        mQuadView = false;
        mHorizontalRatio = 0.5f;
        mVerticalRatio = 0.5f;
    }

    Editor::FEditorViewport* Editor::FEditorViewportLayout::GetViewport(Core::Uint32 Index) const {
        return Index == 0 ? mViewport : Index <= mOrthographicViewports.size() ? mOrthographicViewports[Index - 1].get() : nullptr;
    }

    Editor::FEditorViewport* Editor::FEditorViewportLayout::GetActiveViewport() const {
        return mPlaySession->IsActive() ? mViewport : GetViewport(mActiveViewport);
    }

    void Editor::FEditorViewportLayout::UpdateViewportContexts() {
        Editor::FEditorContext* Context{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};

        for (const auto& Viewport : mOrthographicViewports) {
            if (Viewport != nullptr) {
                Viewport->GetClient()->SetEditorContext(Context);
            }
        }
    }

    bool Editor::FEditorViewportLayout::CanChangeLayout() const {
        if (mPlaySession->IsActive()) {
            return false;
        }

        for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
            Editor::FEditorViewport* Viewport{GetViewport(Index)};

            if (Viewport != nullptr && (Viewport->IsMouseCaptured() || Viewport->GetClient()->GetGizmo().IsDragging())) {
                return false;
            }
        }

        return true;
    }

    void Editor::FEditorViewportLayout::SetQuadView(bool QuadView) {
        if (!CanChangeLayout() || mQuadView == QuadView) {
            return;
        }

        mSingleViewport = mActiveViewport;
        mQuadView = QuadView;

        if (ImGui::GetCurrentContext() != nullptr) {
            ImGui::MarkIniSettingsDirty();
        }
    }

    bool Editor::FEditorViewportLayout::IsQuadView() const {
        return mQuadView;
    }

    void Editor::FEditorViewportLayout::DrawViewport(Core::Uint32 Index, float DeltaTime, Editor::FEditorViewport*& CapturedViewport) {
        Editor::FEditorViewport* Viewport{GetViewport(Index)};

        if (Viewport == nullptr) {
            return;
        }

        Editor::FEditorViewportClient* Client{Viewport->GetClient()};
        const bool Playing{Viewport->GetActiveClient() != Client};
        const bool AllowInput{CapturedViewport == nullptr || CapturedViewport == Viewport};

        Editor::EditorStyle::PushViewportToolbar();

        const float Scale{ImGui::GetStyle().FontScaleMain * ImGui::GetStyle().FontScaleDpi};
        const float LayoutButtonX{ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight()};

        ImGui::BeginDisabled(Playing || !CanChangeLayout());
        ImGui::SetNextItemWidth(std::min(120.0f * Scale, std::max(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x, 60.0f * Scale)));

        if (ImGui::BeginCombo("##ViewType", Playing ? "Play" : Client->GetViewTypeName())) {
            const std::array<const char*, 7> Names{"Perspective", "Top", "Bottom", "Front", "Back", "Left", "Right"};

            for (Core::Uint32 Type{}; Type < Names.size(); ++Type) {
                const Editor::EEditorViewportType ViewType{static_cast<Editor::EEditorViewportType>(Type)};

                if (ImGui::Selectable(Names[Type], Client->GetViewType() == ViewType)) {
                    Client->SetViewType(ViewType);
                    mActiveViewport = Index;
                }
            }

            ImGui::EndCombo();
        }

        ImGui::SameLine(LayoutButtonX);

        if (Editor::EditorStyle::IconButton("ViewportLayout", mQuadView ? Editor::EditorStyle::EIcon::Maximize : Editor::EditorStyle::EIcon::QuadView, false, true, false, mQuadView ? "Maximize Viewport" : "Switch to Four Viewports")) {
            mActiveViewport = Index;
            SetQuadView(!mQuadView);
        }

        ImGui::EndDisabled();
        Editor::EditorStyle::PopViewportToolbar();
        ImGui::Separator();

        Viewport->DrawContents(DeltaTime, AllowInput);
        mVisible[Index] = true;

        if (Viewport->IsMouseCaptured()) {
            CapturedViewport = Viewport;
            mActiveViewport = Index;
        } else if (AllowInput && ImGui::IsWindowFocused()) {
            mActiveViewport = Index;
        }
    }

    void Editor::FEditorViewportLayout::DrawUI(float DeltaTime) {
        UpdateViewportContexts();
        mVisible.fill(false);

        Editor::FEditorViewport* CapturedViewport{};

        for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
            Editor::FEditorViewport* Viewport{GetViewport(Index)};

            if (Viewport != nullptr && Viewport->IsMouseCaptured()) {
                CapturedViewport = Viewport;
                break;
            }
        }

        const ImGuiWindowFlags Flags{ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNavInputs};

        if (ImGui::Begin(mViewport->GetName().c_str(), nullptr, Flags)) {
            const ImVec2 Origin{ImGui::GetCursorScreenPos()};
            const ImVec2 Available{ImGui::GetContentRegionAvail()};
            const bool QuadView{mQuadView};

            if (!QuadView) {
                const Core::Uint32 Index{mPlaySession->IsActive() ? 0 : mSingleViewport};

                if (ImGui::BeginChild("ViewportSingle", Available, ImGuiChildFlags_None, Flags)) {
                    ImGui::PushID(static_cast<int>(Index));
                    DrawViewport(Index, DeltaTime, CapturedViewport);
                    ImGui::PopID();
                }

                ImGui::EndChild();
            } else if (Available.x >= 8.0f && Available.y >= 8.0f) {
                constexpr float SplitterSize{6.0f};
                const ImVec2 Size{Available.x - SplitterSize, Available.y - SplitterSize};
                const float MinimumWidth{std::min(160.0f, Size.x * 0.5f)};
                const float MinimumHeight{std::min(120.0f, Size.y * 0.5f)};
                float Left{std::clamp(Size.x * mHorizontalRatio, MinimumWidth, Size.x - MinimumWidth)};
                float Top{std::clamp(Size.y * mVerticalRatio, MinimumHeight, Size.y - MinimumHeight)};
                float Right{Size.x - Left};
                const bool CanResize{CapturedViewport == nullptr};
                ImGui::BeginDisabled(!CanResize);
                ImGui::SetCursorScreenPos({Origin.x + Left, Origin.y});

                const bool HorizontalChanged{ImGui::SplitterBehavior(ImRect{{Origin.x + Left, Origin.y}, {Origin.x + Left + SplitterSize, Origin.y + Available.y}}, ImGui::GetID("ViewportVerticalSplitter"), ImGuiAxis_X, &Left, &Right, MinimumWidth, MinimumWidth)};

                ImGui::EndDisabled();
                ImGui::BeginDisabled(!CanResize);
                ImGui::SetCursorScreenPos({Origin.x, Origin.y + Top});

                float Bottom{Size.y - Top};
                const bool VerticalChanged{ImGui::SplitterBehavior(ImRect{{Origin.x, Origin.y + Top}, {Origin.x + Available.x, Origin.y + Top + SplitterSize}}, ImGui::GetID("ViewportHorizontalSplitter"), ImGuiAxis_Y, &Top, &Bottom, MinimumHeight, MinimumHeight)};

                ImGui::EndDisabled();

                if (HorizontalChanged || VerticalChanged) {
                    SetSplitRatios(HorizontalChanged ? Left / Size.x : mHorizontalRatio, VerticalChanged ? Top / Size.y : mVerticalRatio);
                }

                const std::array<ImVec2, 4> Positions{{Origin, {Origin.x + Left + SplitterSize, Origin.y}, {Origin.x, Origin.y + Top + SplitterSize}, {Origin.x + Left + SplitterSize, Origin.y + Top + SplitterSize}}};
                const std::array<ImVec2, 4> Sizes{{{Left, Top}, {Size.x - Left, Top}, {Left, Size.y - Top}, {Size.x - Left, Size.y - Top}}};

                for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
                    ImGui::SetCursorScreenPos(Positions[Index]);
                    ImGui::PushID(static_cast<int>(Index));

                    if (ImGui::BeginChild("ViewportPane", Sizes[Index], ImGuiChildFlags_Borders, Flags)) {
                        DrawViewport(Index, DeltaTime, CapturedViewport);
                    }

                    ImGui::EndChild();
                    ImGui::PopID();
                }

                ImGui::SetCursorScreenPos(Origin);
                ImGui::Dummy(Available);
            }
        }

        ImGui::End();

        for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
            if (!mVisible[Index]) {
                if (Editor::FEditorViewport* Viewport{GetViewport(Index)}) {
                    Viewport->ReleaseInput();
                }
            }
        }
    }

    void Editor::FEditorViewportLayout::Draw(float DeltaTime) {
        UpdateViewportContexts();

        for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
            if (mVisible[Index]) {
                GetViewport(Index)->Draw();
            }
        }

        const Editor::FEditorContext* Inspection{mPlaySession->IsActive() ? mPlaySession->GetInspectionContext() : mContext};

        for (Core::Uint32 Index{}; Index < mVisible.size(); ++Index) {
            if (mVisible[Index]) {
                GetViewport(Index)->DrawStatistics(DeltaTime, Inspection->GetWorld(), mContext->GetRenderer()->GetStats());
            }
        }
    }

    float Editor::FEditorViewportLayout::GetHorizontalRatio() const {
        return mHorizontalRatio;
    }

    float Editor::FEditorViewportLayout::GetVerticalRatio() const {
        return mVerticalRatio;
    }

    bool Editor::FEditorViewportLayout::SetSplitRatios(float Horizontal, float Vertical) {
        if (!std::isfinite(Horizontal) || !std::isfinite(Vertical) || Horizontal < 0.01f || Horizontal > 0.99f || Vertical < 0.01f || Vertical > 0.99f) {
            return false;
        }

        if (mHorizontalRatio != Horizontal || mVerticalRatio != Vertical) {
            mHorizontalRatio = Horizontal;
            mVerticalRatio = Vertical;

            if (ImGui::GetCurrentContext() != nullptr) {
                ImGui::MarkIniSettingsDirty();
            }
        }

        return true;
    }

    void* Editor::FEditorViewportLayout::ReadSettingsOpen(ImGuiContext*, ImGuiSettingsHandler* Handler, const char* Name) {
        return std::strcmp(Name, "Layout") == 0 ? Handler->UserData : nullptr;
    }

    void Editor::FEditorViewportLayout::ReadSettingsLine(ImGuiContext*, ImGuiSettingsHandler*, void* Entry, const char* Line) {
        auto* Layout{static_cast<Editor::FEditorViewportLayout*>(Entry)};
        float Ratio{};
        int Value{};

        if (sscanf_s(Line, "HorizontalRatio=%f", &Ratio) == 1) {
            Layout->SetSplitRatios(Ratio, Layout->mVerticalRatio);
        } else if (sscanf_s(Line, "VerticalRatio=%f", &Ratio) == 1) {
            Layout->SetSplitRatios(Layout->mHorizontalRatio, Ratio);
        } else if (sscanf_s(Line, "QuadView=%d", &Value) == 1 && (Value == 0 || Value == 1)) {
            Layout->mQuadView = Value != 0;
        } else if (sscanf_s(Line, "SingleViewport=%d", &Value) == 1 && Value >= 0 && Value < 4) {
            Layout->mSingleViewport = static_cast<Core::Uint32>(Value);
            Layout->mActiveViewport = Layout->mSingleViewport;
        }
    }

    void Editor::FEditorViewportLayout::WriteSettings(ImGuiContext*, ImGuiSettingsHandler* Handler, ImGuiTextBuffer* Buffer) {
        const auto* Layout{static_cast<const Editor::FEditorViewportLayout*>(Handler->UserData)};
        Buffer->appendf("[MacawViewport][Layout]\nHorizontalRatio=%.9g\nVerticalRatio=%.9g\nQuadView=%d\nSingleViewport=%u\n\n", Layout->mHorizontalRatio, Layout->mVerticalRatio, Layout->mQuadView ? 1 : 0, Layout->mSingleViewport);
    }

}
