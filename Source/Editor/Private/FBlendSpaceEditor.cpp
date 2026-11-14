/**
 * @file FBlendSpaceEditor.cpp
 * @brief 블렌드 스페이스 하나를 편집 사본으로 열어 축 범위와 샘플(시퀀스·위치)을 편집하고 저장합니다.
 * @ingroup Editor
 */

#include "Core/Types.h"
#include "FBlendSpaceEditor.h"
#include "Asset/FAssetManager.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace1D.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Editor/FAssetDragDrop.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FEditorAssetCatalog.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

namespace Editor {

    /**
     * @brief 블렌드 스페이스 하나를 편집 사본으로 열어 축 범위와 샘플(시퀀스·위치)을 편집하고 저장하는 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 표시 이름과 무관하게 창 상태를 유지하는 ImGui 창 ID입니다.
         */
        constexpr char WindowId[]{"###BlendSpaceEditor"};

        /**
         * @brief 정규화한 경로가 지정 루트 내부에 속하는지 검사합니다.
         * @param Path 처리할 에셋 또는 파일 경로입니다.
         * @param Root 해당 상태 또는 그래프의 비소유 루트 포즈 노드입니다.
         * @return 경로가 루트 또는 그 하위에 있으면 true입니다.
         */
        bool IsUnder(const std::filesystem::path& Path, const std::filesystem::path& Root) {
            const auto Relative{Path.lexically_normal().lexically_relative(Root.lexically_normal())};

            return !Relative.empty() && *Relative.begin() != L"..";
        }

        // 시퀀스가 지정된 샘플 이름들이 공유하는 접두어의 길이를 마지막 밑줄 다음까지로 구합니다. 샘플이 둘 미만이면 0입니다.

        /**
         * @brief 샘플 에셋 이름들의 공통 접두사 길이를 계산합니다.
         * @param Samples 표시 이름을 비교할 블렌드 샘플 목록입니다.
         * @return 표시 이름에서 생략할 공통 문자 수입니다.
         */
        std::size_t CommonNamePrefix(const Core::TArray<Asset::FBlendSample>& Samples) {
            Core::TArray<Core::FString> Names{};

            for (const Asset::FBlendSample& Sample : Samples) {
                if (Sample.mAnimation != nullptr) {
                    Names.push_back(Sample.mAnimation->GetName());
                }
            }

            if (Names.size() < 2) {
                return 0;
            }

            std::size_t Length{Names[0].size()};

            for (const Core::FString& Name : Names) {
                Length = std::min<std::size_t>(Length, static_cast<std::size_t>(std::mismatch(Names[0].begin(), Names[0].begin() + static_cast<std::ptrdiff_t>(std::min(Length, Name.size())), Name.begin()).first - Names[0].begin()));
            }

            const std::size_t Cut{Length == 0 ? Core::FString::npos : Names[0].rfind('_', Length - 1)};

            return Cut == Core::FString::npos ? 0 : Cut + 1;
        }

        /**
         * @brief 샘플 경로에서 공통 접두사를 생략한 표시 이름을 만듭니다.
         * @param Sample 표시 이름을 만들 샘플입니다.
         * @param Prefix 이름에서 생략할 공통 접두사 길이입니다.
         * @return 샘플 목록에 표시할 이름입니다.
         */
        Core::FString SampleName(const Asset::FBlendSample& Sample, std::size_t Prefix = 0) {
            if (Sample.mAnimation != nullptr) {
                const Core::FString& Name{Sample.mAnimation->GetName()};

                return Prefix < Name.size() ? Name.substr(Prefix) : Name;
            }

            return "(없음)";
        }
    }

    FBlendSpaceEditor::FBlendSpaceEditor(Asset::FAssetManager* Assets, Editor::FEditorAssetCatalog* Catalog, Editor::FAssetThumbnailCache* Thumbnails)
        : mAssets{Assets},
          mCatalog{Catalog},
          mThumbnails{Thumbnails} {
    }

    FBlendSpaceEditor::~FBlendSpaceEditor() = default;

    bool FBlendSpaceEditor::Open(const std::filesystem::path& Path, Core::FString& Error) {
        Asset::UBlendSpace* Source{mAssets != nullptr ? mAssets->LoadBlendSpace(Path, Error) : nullptr};

        if (Source == nullptr) {
            return false;
        }

        mEditing = CoreUObject::NewObject<Asset::UBlendSpace1D>();
        mEditing->CopyFrom(*Source);
        mEditing->SetName(Source->GetName());

        mPath = Path;
        mDirty = false;
        mOpen = true;
        mError.clear();
        mSelectedSample = -1;
        mDraggingSample = -1;
        mDraggingPreview = false;
        mRevealSample = false;

        const Asset::FBlendParameter& Parameter{mEditing->GetBlendParameter(Asset::EBlendSpaceAxis::X)};

        mPreviewInput = std::clamp(mPreviewInput, Parameter.mMin, Parameter.mMax);
        ImGui::SetWindowFocus(WindowId);

        return true;
    }

    bool FBlendSpaceEditor::IsOpen() const {
        return mOpen;
    }

    bool FBlendSpaceEditor::IsFocused() const {
        ImGuiWindow* Window{mOpen ? ImGui::FindWindowByName(WindowId) : nullptr};
        ImGuiWindow* Focused{ImGui::GetCurrentContext()->NavWindow};

        return Window != nullptr && Focused != nullptr && ImGui::IsWindowChildOf(Focused, Window, true, false);
    }

    void FBlendSpaceEditor::CloseRetired(const std::filesystem::path& Path) {
        if (!mOpen || mAssets == nullptr) {
            return;
        }

        const std::filesystem::path Root{mAssets->GetContentRoot()};
        const std::filesystem::path Target{Path.is_absolute() ? Path : Root / Path};

        if (Root / mPath == Target || IsUnder(Root / mPath, Target)) {
            Close();
        }
    }

    void FBlendSpaceEditor::Close() {
        mOpen = false;
        mEditing.reset();
    }

    bool FBlendSpaceEditor::Save() {
        if (mEditing == nullptr || mAssets == nullptr) {
            return false;
        }

        if (!mAssets->SaveBlendSpace(mPath, *mEditing, mError)) {
            return false;
        }

        mDirty = false;
        mError.clear();

        return true;
    }


    float FBlendSpaceEditor::Snap(float Value, bool Free) const {
        const Asset::FBlendParameter& Parameter{mEditing->GetBlendParameter(Asset::EBlendSpaceAxis::X)};
        float Result{std::clamp(Value, Parameter.mMin, Parameter.mMax)};

        if (mSnapToGrid && !Free) {
            const float Step{(Parameter.mMax - Parameter.mMin) / static_cast<float>(std::max(Parameter.mGridDivisions, 1))};

            Result = Parameter.mMin + std::round((Result - Parameter.mMin) / Step) * Step;
        }

        return std::clamp(Result, Parameter.mMin, Parameter.mMax);
    }

    void FBlendSpaceEditor::AddSampleAt(const Core::FString& AnimationPath, float Value) {
        Core::FString Error{};
        const Asset::UAnimSequence* Animation{AnimationPath.empty() || mAssets == nullptr ? nullptr : mAssets->LoadAnimSequence(Core::FilePath::FromUtf8(AnimationPath), Error)};
        const Core::Int32 Index{mEditing->AddSample(Animation, {Value, 0.0f, 0.0f})};

        if (Index < 0) {
            mError = "샘플 수가 한도에 도달했습니다.";

            return;
        }

        mSelectedSample = Index;
        mRevealSample = true;
        mDirty = true;
        mError = Error;
    }

    void FBlendSpaceEditor::Draw() {
        if (!mOpen || mEditing == nullptr) {
            return;
        }

        const Core::FString Title{std::format("Blend Space - {}{}{}", mEditing->GetName(), mDirty ? " *" : "", WindowId)};
        bool Visible{true};

        ImGui::SetNextWindowSize({960.0f, 640.0f}, ImGuiCond_FirstUseEver);

        const bool Expanded{ImGui::Begin(Title.c_str(), &Visible)};

        if (!Visible) {
            ImGui::End();
            Close();

            return;
        }

        if (Expanded) {
            ImGui::Checkbox("격자에 맞춤", &mSnapToGrid);
            ImGui::SameLine();
            ImGui::Checkbox("샘플 이름", &mShowSampleNames);

            if (!mError.empty()) {
                ImGui::TextColored({1.0f, 0.45f, 0.4f, 1.0f}, "%s", mError.c_str());
            }

            DrawCanvas();

            ImGui::Spacing();

            if (ImGui::BeginTable("BlendSpaceDetails", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV, ImGui::GetContentRegionAvail())) {
                ImGui::TableSetupColumn("Axis", ImGuiTableColumnFlags_WidthFixed, 240.0f);
                ImGui::TableSetupColumn("Samples", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);

                if (ImGui::BeginChild("BlendAxis")) {
                    DrawAxis();
                }

                ImGui::EndChild();
                ImGui::TableSetColumnIndex(1);

                if (ImGui::BeginChild("BlendSamples")) {
                    DrawSamples();
                }

                ImGui::EndChild();
                ImGui::EndTable();
            }
        }

        if (mDirty && IsFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S)) {
            Save();
        }

        ImGui::End();
    }

    void FBlendSpaceEditor::DrawAxis() {
        ImGui::SeparatorText("Axis");

        Asset::FBlendParameter Parameter{mEditing->GetBlendParameter(Asset::EBlendSpaceAxis::X)};
        std::array<char, 64> Name{};

        std::copy_n(Parameter.mDisplayName.begin(), std::min(Parameter.mDisplayName.size(), Name.size() - 1), Name.begin());

        bool Changed{ImGui::InputText("이름", Name.data(), Name.size())};

        Changed |= ImGui::DragFloat("최솟값", &Parameter.mMin, 0.05f, 0.0f, 0.0f, "%.2f");
        Changed |= ImGui::DragFloat("최댓값", &Parameter.mMax, 0.05f, 0.0f, 0.0f, "%.2f");
        Changed |= ImGui::SliderInt("격자 칸 수", &Parameter.mGridDivisions, 1, 32);

        if (Changed) {
            Parameter.mDisplayName = Name.data();

            if (mEditing->SetBlendParameter(Asset::EBlendSpaceAxis::X, Parameter)) {
                mDirty = true;
                mError.clear();
                mPreviewInput = std::clamp(mPreviewInput, Parameter.mMin, Parameter.mMax);
            } else {
                mError = "축 최솟값은 최댓값보다 작아야 합니다.";
            }
        }

        ImGui::Spacing();
        ImGui::TextDisabled("격자 간격 %.3f", (Parameter.mMax - Parameter.mMin) / static_cast<float>(std::max(Parameter.mGridDivisions, 1)));
    }

    void FBlendSpaceEditor::DrawSamples() {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Samples  %zu", mEditing->GetSamples().size());
        ImGui::SameLine();
        ImGui::BeginDisabled(mEditing->GetSamples().size() >= Asset::UBlendSpace::MaxSamples);

        if (ImGui::Button("+ 빈 샘플")) {
            AddSampleAt({}, Snap(mPreviewInput, false));
        }

        ImGui::EndDisabled();

        const Asset::FBlendParameter& Parameter{mEditing->GetBlendParameter(Asset::EBlendSpaceAxis::X)};
        const auto& Samples{mEditing->GetSamples()};
        const std::size_t CommonPrefix{CommonNamePrefix(Samples)};
        const float RowHeight{ImGui::GetFrameHeight() + 2.0f * ImGui::GetStyle().CellPadding.y};
        Core::Int32 Remove{-1};

        if (ImGui::BeginTable("SampleList", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail())) {
            ImGui::TableSetupColumn("Animation", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn(Parameter.mDisplayName.c_str(), ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("##Remove", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFrameHeight());
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            for (Core::Index_t Index{}; Index < Samples.size(); ++Index) {
                const Asset::FBlendSample& Sample{Samples[Index]};
                const bool Selected{mSelectedSample == Index};

                ImGui::PushID(Index);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, RowHeight);

                if (Selected) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, ImGui::GetColorU32(ImGuiCol_Header));
                }

                if (ImGui::TableSetColumnIndex(0)) {
                    Core::FString AnimationPath{Sample.mAnimation != nullptr ? Sample.mAnimation->GetAssetPath() : Core::FString{}};
                    const Core::FString Name{SampleName(Sample, CommonPrefix)};

                    if (mCatalog != nullptr && mThumbnails != nullptr) {
                        if (mPicker.DrawCompact(*mCatalog, *mThumbnails, "##Animation", Name.c_str(), AnimationPath, Asset::EAssetType::AnimSequence)) {
                            Core::FString Error{};
                            const Asset::UAnimSequence* Animation{AnimationPath.empty() ? nullptr : mAssets->LoadAnimSequence(Core::FilePath::FromUtf8(AnimationPath), Error)};

                            mEditing->SetSampleAnimation(Index, Animation);
                            mSelectedSample = Index;
                            mDirty = true;
                            mError = Error;
                        }

                        if (ImGui::IsItemClicked()) {
                            mSelectedSample = Index;
                        }
                    } else {
                        ImGui::TextUnformatted(Name.c_str());
                    }
                }

                if (ImGui::TableSetColumnIndex(1)) {
                    float Value{Sample.mSampleValue.x};

                    ImGui::SetNextItemWidth(-1.0f);

                    if (ImGui::DragFloat("##Value", &Value, 0.01f, Parameter.mMin, Parameter.mMax, "%.4g")) {
                        mEditing->SetSampleValue(Index, {std::clamp(Value, Parameter.mMin, Parameter.mMax), 0.0f, 0.0f});
                        mSelectedSample = Index;
                        mDirty = true;
                    }
                }

                if (ImGui::TableSetColumnIndex(2) && ImGui::Button("×")) {
                    Remove = Index;
                }

                if (Selected && mRevealSample) {
                    ImGui::SetScrollFromPosY(ImGui::GetCursorPosY() + RowHeight * 0.5f, 0.5f);
                    mRevealSample = false;
                }

                ImGui::PopID();
            }

            ImGui::EndTable();
        }

        if (Remove >= 0) {
            mEditing->RemoveSample(Remove);
            mSelectedSample = -1;
            mDirty = true;
        }
    }

    void FBlendSpaceEditor::DrawCanvas() {
        const Asset::FBlendParameter& Parameter{mEditing->GetBlendParameter(Asset::EBlendSpaceAxis::X)};
        const auto& Samples{mEditing->GetSamples()};
        const ImGuiIO& IO{ImGui::GetIO()};

        const ImVec2 Origin{ImGui::GetCursorScreenPos()};
        const float TextHeight{ImGui::GetTextLineHeight()};
        const ImVec2 Size{std::max(ImGui::GetContentRegionAvail().x, 1.0f), 7.5f * TextHeight};
        const ImVec2 End{Origin.x + Size.x, Origin.y + Size.y};
        const Core::FString MinText{std::format("{:.5g}", Parameter.mMin)};
        const Core::FString MaxText{std::format("{:.5g}", Parameter.mMax)};
        const float TickWidth{std::max(ImGui::CalcTextSize(MinText.c_str()).x, ImGui::CalcTextSize(MaxText.c_str()).x)};
        const float Margin{std::min(Size.x * 0.25f, std::max(TextHeight * 1.6f, 0.5f * TickWidth + 8.0f))};
        const float TrackWidth{Size.x - 2.0f * Margin};
        const float AxisY{End.y - 2.5f * TextHeight};
        const double Range{static_cast<double>(Parameter.mMax) - Parameter.mMin};
        const auto ToScreen{[&](float Value) {
            const double Alpha{std::clamp((static_cast<double>(Value) - Parameter.mMin) / Range, 0.0, 1.0)};

            return Origin.x + Margin + TrackWidth * static_cast<float>(Alpha);
        }};
        const auto ToValue{[&](float ScreenX) {
            const float Alpha{std::clamp((ScreenX - Origin.x - Margin) / TrackWidth, 0.0f, 1.0f)};

            return static_cast<float>(Parameter.mMin + Alpha * Range);
        }};

        ImGui::InvisibleButton("BlendSpaceCanvas", Size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

        const bool CanvasHovered{ImGui::IsItemHovered()};
        const ImVec2 Mouse{IO.MousePos};

        // 콘텐츠 브라우저에서 끌어 온 시퀀스를 놓은 위치에 샘플로 추가합니다.
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(Editor::EditorAssetDragDrop::AnimSequencePayload)}; Payload != nullptr && Payload->Data != nullptr && Payload->DataSize > 1 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                const Core::FString Candidate{static_cast<const char*>(Payload->Data), static_cast<std::size_t>(Payload->DataSize - 1)};
                const auto* Entry{mCatalog != nullptr ? mCatalog->Find(Core::FilePath::FromUtf8(Candidate)) : nullptr};

                if (Entry != nullptr && Entry->GetType() == Asset::EAssetType::AnimSequence) {
                    AddSampleAt(Core::FilePath::ToGenericUtf8(Entry->GetPath()), Snap(ToValue(Mouse.x), IO.KeyCtrl));
                }
            }

            ImGui::EndDragDropTarget();
        }

        // 마우스 아래 샘플을 찾습니다. 축 근처에서 가장 가까운 마름모를 고릅니다.
        Core::Int32 Hovered{-1};
        float HoveredDistance{TextHeight * 0.75f};

        if (CanvasHovered && std::abs(Mouse.y - AxisY) < TextHeight) {
            for (Core::Index_t Index{}; Index < Samples.size(); ++Index) {
                const float Distance{std::abs(Mouse.x - ToScreen(Samples[Index].mSampleValue.x))};

                if (Distance < HoveredDistance || (Distance == HoveredDistance && Index == mSelectedSample)) {
                    HoveredDistance = Distance;
                    Hovered = Index;
                }
            }
        }

        // 왼쪽 버튼: 샘플 위에서 누르면 그 샘플을 끌고, 빈 곳에서 누르면 미리보기 입력을 끕니다.
        if (CanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (Hovered >= 0) {
                mSelectedSample = Hovered;
                mDraggingSample = Hovered;
                mRevealSample = true;
            } else {
                mDraggingPreview = true;
            }
        }

        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            mDraggingSample = -1;
            mDraggingPreview = false;
        }

        if (mDraggingSample >= 0 && static_cast<Core::Index_t>(mDraggingSample) < Samples.size()) {
            const float Value{Snap(ToValue(Mouse.x), IO.KeyCtrl)};

            if (Value != Samples[static_cast<Core::Index_t>(mDraggingSample)].mSampleValue.x) {
                mEditing->SetSampleValue(mDraggingSample, {Value, 0.0f, 0.0f});
                mDirty = true;
            }
        }

        if (mDraggingPreview || (CanvasHovered && IO.KeyCtrl && mDraggingSample < 0)) {
            mPreviewInput = ToValue(Mouse.x);
        }

        // 오른쪽 버튼: 샘플 위면 샘플 메뉴, 빈 곳이면 그 위치에 샘플을 추가하는 메뉴를 엽니다.
        if (CanvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            if (Hovered >= 0) {
                mSelectedSample = Hovered;
                mRevealSample = true;
                ImGui::OpenPopup("BlendSampleMenu");
            } else {
                mAddValue = Snap(ToValue(Mouse.x), IO.KeyCtrl);
                mAddSearch.fill('\0');
                ImGui::OpenPopup("BlendAddSample");
            }
        }

        ImDrawList* DrawList{ImGui::GetWindowDrawList()};
        const ImU32 GridColor{IM_COL32(49, 62, 79, 255)};
        const ImU32 TextColor{ImGui::GetColorU32(ImGuiCol_Text)};
        const ImU32 TickColor{IM_COL32(163, 179, 198, 255)};
        const ImU32 SampleColor{IM_COL32(185, 199, 219, 255)};
        const ImU32 ActiveColor{IM_COL32(255, 193, 100, 255)};
        const ImU32 PreviewColor{IM_COL32(105, 235, 169, 255)};
        const ImU32 SelectionColor{IM_COL32(111, 195, 255, 255)};

        Core::TArray<Asset::FBlendSampleWeight> Weights{};
        mEditing->GetSamplesToBlend({mPreviewInput, 0.0f, 0.0f}, Weights);

        DrawList->PushClipRect(Origin, End, true);
        DrawList->AddRectFilled(Origin, End, IM_COL32(19, 25, 34, 255), 6.0f);
        DrawList->AddRect(Origin, End, CanvasHovered ? IM_COL32(95, 113, 139, 255) : GridColor, 6.0f);

        const Core::Int32 Divisions{std::max(Parameter.mGridDivisions, 1)};
        const float TickGap{TickWidth + TextHeight};
        const float TrackEnd{ToScreen(Parameter.mMax)};
        float LastTickX{ToScreen(Parameter.mMin) - TickGap};

        for (Core::Int32 Line{}; Line <= Divisions; ++Line) {
            const float Value{static_cast<float>(Parameter.mMin + Range * Line / Divisions)};
            const float X{ToScreen(Value)};
            const bool Endpoint{Line == 0 || Line == Divisions};

            DrawList->AddLine({X, Origin.y + TextHeight * 2.2f}, {X, AxisY + 5.0f}, GridColor, Endpoint ? 1.5f : 1.0f);

            if (Endpoint || (X - LastTickX >= TickGap && TrackEnd - X >= TickGap)) {
                const Core::FString Label{std::format("{:.5g}", Value)};
                const float HalfWidth{0.5f * ImGui::CalcTextSize(Label.c_str()).x};

                DrawList->AddText({X - HalfWidth, AxisY + 0.7f * TextHeight}, TickColor, Label.c_str());
                LastTickX = X;
            }
        }

        DrawList->AddLine({ToScreen(Parameter.mMin), AxisY}, {TrackEnd, AxisY}, TickColor, 2.0f);

        if (Weights.size() == 2) {
            const float Left{ToScreen(Samples[static_cast<std::size_t>(Weights.front().mSampleIndex)].mSampleValue.x)};
            const float Right{ToScreen(Samples[static_cast<std::size_t>(Weights.back().mSampleIndex)].mSampleValue.x)};

            DrawList->AddLine({Left, AxisY}, {Right, AxisY}, ActiveColor, 3.0f);
        }

        const float PreviewX{ToScreen(mPreviewInput)};
        const Core::FString PreviewLabel{std::format("{:.5g}", mPreviewInput)};
        const ImVec2 PreviewSize{ImGui::CalcTextSize(PreviewLabel.c_str())};
        const float PreviewLabelX{std::clamp(PreviewX - PreviewSize.x * 0.5f, Origin.x + 8.0f, std::max(Origin.x + 8.0f, End.x - PreviewSize.x - 8.0f))};
        const float LabelY{Origin.y + 0.5f * TextHeight};

        DrawList->AddLine({PreviewX, LabelY + TextHeight + 4.0f}, {PreviewX, AxisY + 6.0f}, PreviewColor, 2.0f);
        DrawList->AddRectFilled({PreviewLabelX - 6.0f, LabelY - 3.0f}, {PreviewLabelX + PreviewSize.x + 6.0f, LabelY + TextHeight + 3.0f}, IM_COL32(26, 65, 58, 255), 5.0f);
        DrawList->AddText({PreviewLabelX, LabelY}, PreviewColor, PreviewLabel.c_str());
        DrawList->AddText({Origin.x + Margin, End.y - 1.15f * TextHeight}, TextColor, Parameter.mDisplayName.c_str());

        Core::TArray<Core::Index_t> Order(Samples.size());
        std::array<float, Asset::UBlendSpace::MaxSamples> SampleWeights{};

        for (const Asset::FBlendSampleWeight& Weight : Weights) {
            SampleWeights[static_cast<Core::Index_t>(Weight.mSampleIndex)] = Weight.mWeight;
        }

        for (Core::Index_t Index{}; Index < Samples.size(); ++Index) {
            Order[Index] = Index;
        }

        const auto Priority{[&](Core::Index_t Index) {
            return Index == Hovered || Index == mSelectedSample ? 2 : SampleWeights[Index] > 0.0f ? 1 : 0;
        }};
        std::stable_sort(Order.begin(), Order.end(), [&Priority](Core::Index_t Left, Core::Index_t Right) {
            return Priority(Left) > Priority(Right);
        });

        const std::size_t CommonPrefix{CommonNamePrefix(Samples)};
        std::array<Core::TArray<ImVec2>, 2> LabelBounds{};

        for (const Core::Index_t Index : Order) {
            const auto SampleIndex{Index};
            const Asset::FBlendSample& Sample{Samples[SampleIndex]};
            const float X{ToScreen(Sample.mSampleValue.x)};
            const float Weight{SampleWeights[SampleIndex]};
            const bool Highlight{Priority(Index) == 2};
            const float Radius{TextHeight * (Highlight ? 0.49f : 0.36f)};
            const ImU32 Fill{Weight > 0.0f ? ActiveColor : Sample.mAnimation == nullptr ? IM_COL32(200, 90, 80, 255) : SampleColor};

            DrawList->AddQuadFilled({X, AxisY - Radius}, {X + Radius, AxisY}, {X, AxisY + Radius}, {X - Radius, AxisY}, Fill);

            if (Highlight) {
                DrawList->AddQuad({X, AxisY - Radius - 2.0f}, {X + Radius + 2.0f, AxisY}, {X, AxisY + Radius + 2.0f}, {X - Radius - 2.0f, AxisY}, SelectionColor, 2.0f);
            }

            if (mShowSampleNames) {
                const Core::FString Name{SampleName(Sample, CommonPrefix)};
                const float NameWidth{ImGui::CalcTextSize(Name.c_str()).x};
                const float Left{std::clamp(X - NameWidth * 0.5f, Origin.x + 6.0f, std::max(Origin.x + 6.0f, End.x - NameWidth - 6.0f))};
                const ImVec2 Bounds{Left - 5.0f, Left + NameWidth + 5.0f};

                for (Core::Index_t Lane{}; Lane < LabelBounds.size(); ++Lane) {
                    const auto& Placed{LabelBounds[Lane]};

                    if (Bounds.y > End.x || std::any_of(Placed.begin(), Placed.end(), [Bounds](const ImVec2& Other) {
                        return Bounds.x < Other.y && Other.x < Bounds.y;
                    })) {
                        continue;
                    }

                    const float Y{AxisY - TextHeight * (1.55f + static_cast<float>(Lane) * 1.25f)};

                    DrawList->AddRectFilled({Bounds.x, Y - 1.0f}, {Bounds.y, Y + TextHeight + 1.0f}, IM_COL32(26, 34, 47, 255), 3.0f);
                    DrawList->AddText({Left, Y}, Highlight ? SelectionColor : Weight > 0.0f ? ActiveColor : TextColor, Name.c_str());
                    LabelBounds[Lane].push_back(Bounds);

                    break;
                }
            }

            if (Index == Hovered && mDraggingSample < 0) {
                ImGui::SetTooltip("%s\n%s = %.4g   ·   %.1f%%", SampleName(Sample).c_str(), Parameter.mDisplayName.c_str(), Sample.mSampleValue.x, Weight * 100.0f);
            }
        }

        DrawList->PopClipRect();

        if (mDraggingSample >= 0 && static_cast<Core::Index_t>(mDraggingSample) < Samples.size()) {
            ImGui::SetTooltip("%s = %.4g", Parameter.mDisplayName.c_str(), Samples[static_cast<Core::Index_t>(mDraggingSample)].mSampleValue.x);
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Preview Weights");
        ImGui::SameLine();
        ImGui::TextDisabled("%.2f s", mEditing->GetBlendedPlayLength(Weights));

        if (ImGui::BeginTable("PreviewWeights", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Sample", ImGuiTableColumnFlags_WidthStretch, 0.55f);
            ImGui::TableSetupColumn("Weight", ImGuiTableColumnFlags_WidthStretch, 0.45f);
            ImGui::TableSetupColumn("Percent", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("100.0%").x);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ActiveColor);

            for (Core::Index_t Index{}; Index < 2; ++Index) {
                ImGui::TableNextRow(ImGuiTableRowFlags_None, TextHeight + 2.0f * ImGui::GetStyle().CellPadding.y);
                ImGui::TableSetColumnIndex(0);

                if (Index < Weights.size()) {
                    const Asset::FBlendSampleWeight& Weight{Weights[Index]};
                    const Asset::FBlendSample& Sample{Samples[static_cast<Core::Index_t>(Weight.mSampleIndex)]};

                    ImGui::TextUnformatted(SampleName(Sample, CommonPrefix).c_str());
                    ImGui::SetItemTooltip("%s", SampleName(Sample).c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::ProgressBar(Weight.mWeight, {-1.0f, TextHeight}, "");
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.1f%%", Weight.mWeight * 100.0f);
                } else if (Index == 0) {
                    ImGui::TextDisabled("—");
                }
            }

            ImGui::PopStyleColor();
            ImGui::EndTable();
        }

        DrawSampleMenus();
    }

    void FBlendSpaceEditor::DrawSampleMenus() {
        if (ImGui::BeginPopup("BlendSampleMenu")) {
            const bool Valid{mSelectedSample >= 0 && static_cast<Core::Index_t>(mSelectedSample) < mEditing->GetSamples().size()};

            if (ImGui::MenuItem("미리보기를 이 샘플로", nullptr, false, Valid)) {
                mPreviewInput = mEditing->GetSamples()[static_cast<Core::Index_t>(mSelectedSample)].mSampleValue.x;
            }

            if (ImGui::MenuItem("샘플 삭제", nullptr, false, Valid)) {
                mEditing->RemoveSample(mSelectedSample);
                mSelectedSample = -1;
                mDirty = true;
            }

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("BlendAddSample")) {
            if (ImGui::IsWindowAppearing()) {
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth(280.0f);
            ImGui::InputTextWithHint("##Search", "시퀀스 검색", mAddSearch.data(), mAddSearch.size());
            ImGui::Separator();

            const ImGuiTextFilter Filter{mAddSearch.data()};

            if (ImGui::BeginChild("##Sequences", {280.0f, 240.0f})) {
                if (mCatalog != nullptr) {
                    for (const auto& Entry : mCatalog->GetEntries()) {
                        const Core::FString Path{Core::FilePath::ToGenericUtf8(Entry.GetPath())};

                        if (Entry.GetType() != Asset::EAssetType::AnimSequence || !Filter.PassFilter(Path.c_str())) {
                            continue;
                        }

                        if (ImGui::Selectable(Core::FilePath::ToUtf8(Entry.GetPath().stem()).c_str())) {
                            AddSampleAt(Path, mAddValue);
                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", Path.c_str());
                        }
                    }
                }
            }

            ImGui::EndChild();
            ImGui::EndPopup();
        }

    }

}
