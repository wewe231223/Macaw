#include "FAnimSequenceTimeline.h"

#include "FAnimationPreviewScene.h"
#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>
#include <imgui.h>
#include <imgui_neo_sequencer.h>

using namespace Editor;

namespace {

    enum class ETransportIcon {
        ToStart,
        PreviousFrame,
        PlayReverse,
        Pause,
        Play,
        NextFrame,
        ToEnd,
        Loop
    };

    void DrawTriangle(ImDrawList* DrawList, const ImVec2& Center, float Width, float Height, bool Right, bool Filled, ImU32 Color) {
        const float Direction{Right ? 1.0f : -1.0f};
        const ImVec2 A{Center.x + Direction * Width * 0.5f, Center.y};
        const ImVec2 B{Center.x - Direction * Width * 0.5f, Center.y - Height * 0.5f};
        const ImVec2 C{Center.x - Direction * Width * 0.5f, Center.y + Height * 0.5f};

        if (Filled) {
            DrawList->AddTriangleFilled(A, B, C, Color);
        } else {
            DrawList->AddTriangle(A, B, C, Color, 1.5f);
        }
    }

    void DrawBar(ImDrawList* DrawList, float X, float Y, float Width, float Height, ImU32 Color) {
        DrawList->AddRectFilled({X - Width * 0.5f, Y - Height * 0.5f}, {X + Width * 0.5f, Y + Height * 0.5f}, Color);
    }

    void DrawTransportIcon(ImDrawList* DrawList, ETransportIcon Icon, const ImVec2& Center, float Size, ImU32 Color) {
        const float BarWidth{std::max(1.5f, std::round(Size * 0.14f))};

        switch (Icon) {
        case ETransportIcon::ToStart:
            DrawBar(DrawList, Center.x - Size * 0.38f, Center.y, BarWidth, Size * 0.8f, Color);
            DrawTriangle(DrawList, {Center.x + Size * 0.08f, Center.y}, Size * 0.62f, Size * 0.8f, false, true, Color);
            break;
        case ETransportIcon::PreviousFrame:
            DrawTriangle(DrawList, {Center.x - Size * 0.1f, Center.y}, Size * 0.55f, Size * 0.75f, false, false, Color);
            DrawBar(DrawList, Center.x + Size * 0.32f, Center.y, BarWidth, Size * 0.8f, Color);
            break;
        case ETransportIcon::PlayReverse:
            DrawTriangle(DrawList, Center, Size * 0.7f, Size * 0.85f, false, true, Color);
            break;
        case ETransportIcon::Pause:
            DrawBar(DrawList, Center.x - Size * 0.17f, Center.y, BarWidth * 1.4f, Size * 0.8f, Color);
            DrawBar(DrawList, Center.x + Size * 0.17f, Center.y, BarWidth * 1.4f, Size * 0.8f, Color);
            break;
        case ETransportIcon::Play:
            DrawTriangle(DrawList, Center, Size * 0.7f, Size * 0.85f, true, true, Color);
            break;
        case ETransportIcon::NextFrame:
            DrawBar(DrawList, Center.x - Size * 0.32f, Center.y, BarWidth, Size * 0.8f, Color);
            DrawTriangle(DrawList, {Center.x + Size * 0.1f, Center.y}, Size * 0.55f, Size * 0.75f, true, false, Color);
            break;
        case ETransportIcon::ToEnd:
            DrawTriangle(DrawList, {Center.x - Size * 0.08f, Center.y}, Size * 0.62f, Size * 0.8f, true, true, Color);
            DrawBar(DrawList, Center.x + Size * 0.38f, Center.y, BarWidth, Size * 0.8f, Color);
            break;
        case ETransportIcon::Loop: {
            const float Radius{Size * 0.14f};
            const ImVec2 Minimum{Center.x - Size * 0.42f, Center.y - Size * 0.3f};
            const ImVec2 Maximum{Center.x + Size * 0.42f, Center.y + Size * 0.3f};
            DrawList->PathLineTo({Center.x - Size * 0.05f, Maximum.y});
            DrawList->PathArcTo({Minimum.x + Radius, Maximum.y - Radius}, Radius, std::numbers::pi_v<float> * 0.5f, std::numbers::pi_v<float>, 4);
            DrawList->PathArcTo({Minimum.x + Radius, Minimum.y + Radius}, Radius, std::numbers::pi_v<float>, std::numbers::pi_v<float> * 1.5f, 4);
            DrawList->PathArcTo({Maximum.x - Radius, Minimum.y + Radius}, Radius, std::numbers::pi_v<float> * 1.5f, std::numbers::pi_v<float> * 2.0f, 4);
            DrawList->PathArcTo({Maximum.x - Radius, Maximum.y - Radius}, Radius, 0.0f, std::numbers::pi_v<float> * 0.5f, 4);
            DrawList->PathLineTo({Center.x + Size * 0.12f, Maximum.y});
            DrawList->PathStroke(Color, 0, std::max(1.5f, Size * 0.11f));
            DrawTriangle(DrawList, {Center.x + Size * 0.02f, Maximum.y}, Size * 0.26f, Size * 0.3f, false, true, Color);
            break;
        }
        }
    }

    bool TransportButton(const char* Id, ETransportIcon Icon, bool Active, const char* Tooltip) {
        const float Size{ImGui::GetFrameHeight()};
        const bool Pressed{ImGui::InvisibleButton(Id, {Size, Size})};
        const bool Hovered{ImGui::IsItemHovered()};
        const ImVec2 Minimum{ImGui::GetItemRectMin()};
        const ImVec2 Maximum{ImGui::GetItemRectMax()};
        ImDrawList* DrawList{ImGui::GetWindowDrawList()};

        if (Hovered) {
            DrawList->AddRectFilled(Minimum, Maximum, ImGui::GetColorU32(ImVec4{1.0f, 1.0f, 1.0f, ImGui::IsItemActive() ? 0.16f : 0.08f}), 3.0f);
            ImGui::SetTooltip("%s", Tooltip);
        }

        const ImU32 Color{ImGui::GetColorU32(Active ? ImVec4{0.22f, 0.63f, 1.0f, 1.0f} : Hovered ? ImVec4{1.0f, 1.0f, 1.0f, 1.0f} : ImVec4{0.78f, 0.78f, 0.78f, 1.0f})};
        DrawTransportIcon(DrawList, Icon, {(Minimum.x + Maximum.x) * 0.5f, (Minimum.y + Maximum.y) * 0.5f}, Size * 0.6f, Color);

        return Pressed;
    }

}

FAnimSequenceTimeline::FAnimSequenceTimeline() = default;

FAnimSequenceTimeline::~FAnimSequenceTimeline() = default;

void FAnimSequenceTimeline::Reset(const Asset::UAnimSequence& Sequence) {
    mEndFrame = std::max(static_cast<Core::Int32>(Sequence.GetDataModel().GetNumberOfFrames()), 1);
    ++mSequenceRevision;
}

void FAnimSequenceTimeline::Draw(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, float Height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(24, 24, 24, 255));

    if (ImGui::BeginChild("##AnimationTimeline", {0.0f, Height}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const float BarHeight{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y};
        DrawTimeArea(Preview, Sequence, std::max(ImGui::GetContentRegionAvail().y - BarHeight, 1.0f));
        DrawTransportBar(Preview, Sequence);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void FAnimSequenceTimeline::DrawTimeArea(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, float Height) {
    if (!ImGui::BeginChild("##AnimationTimeArea", {0.0f, Height}, ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::EndChild();

        return;
    }

    const double Rate{Sequence.GetDataModel().GetFrameRate().AsDecimal()};
    const Core::Int32 CurrentFrame{static_cast<Core::Int32>(std::clamp(std::floor(Preview.GetPosition() * Rate + 0.0001), 0.0, static_cast<double>(mEndFrame)))};
    Core::Int32 Frame{CurrentFrame};
    Core::Int32 StartFrame{};
    Core::Int32 EndFrame{mEndFrame};
    const Core::FString Id{std::format("##AnimationSequence{}", mSequenceRevision)};

    ImGui::PushID(this);

    if (ImGui::BeginNeoSequencer(Id.c_str(), &Frame, &StartFrame, &EndFrame, ImVec2{}, ImGuiNeoSequencerFlags_AlwaysShowHeader)) {
        if (ImGui::BeginNeoTimelineEx("Animation")) {
            ImGui::NeoDrawTimelineRect(0, mEndFrame, ImVec4{0.25f, 0.33f, 0.41f, 0.7f});
            ImGui::EndNeoTimeLine();
        }

        ImGui::EndNeoSequencer();
    }

    ImGui::PopID();

    if (Frame != CurrentFrame && Preview.IsReady()) {
        Preview.Seek(static_cast<double>(std::clamp(Frame, 0, mEndFrame)) / Rate);
    }

    ImGui::EndChild();
}

void FAnimSequenceTimeline::StepFrame(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence, Core::Int32 Direction) {
    const double Rate{Sequence.GetDataModel().GetFrameRate().AsDecimal()};
    const double Frame{Preview.GetPosition() * Rate};
    const double Next{Direction < 0 ? std::ceil(Frame - 0.00001) - 1.0 : std::floor(Frame + 0.00001) + 1.0};

    Preview.Seek(Next / Rate);
}

void FAnimSequenceTimeline::DrawTransportBar(Editor::FAnimationPreviewScene& Preview, const Asset::UAnimSequence& Sequence) {
    const ImGuiStyle& Style{ImGui::GetStyle()};
    const float Height{ImGui::GetFrameHeight() + Style.WindowPadding.y};
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{Style.WindowPadding.x, Style.WindowPadding.y * 0.5f});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2.0f, Style.ItemSpacing.y});

    if (ImGui::BeginChild("##AnimationTransport", {0.0f, Height}, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const float Width{ImGui::GetContentRegionAvail().x};
        const float StartX{ImGui::GetCursorPosX()};
        const float RowY{ImGui::GetCursorPosY()};
        const double Time{Preview.GetPosition()};
        const double Rate{Sequence.GetDataModel().GetFrameRate().AsDecimal()};
        const Core::Int32 Frame{static_cast<Core::Int32>(std::floor(Time * Rate + 0.0001))};

        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%d / %u  (%.3fs)", Frame, Sequence.GetDataModel().GetNumberOfFrames(), Time);

        const bool Reverse{Preview.IsPlaying() && Preview.GetPlayRate() < 0.0f};
        const bool Forward{Preview.IsPlaying() && !Reverse};
        const float ButtonsWidth{ImGui::GetFrameHeight() * 7.0f + ImGui::GetStyle().ItemSpacing.x * 6.0f};

        ImGui::SameLine();
        ImGui::SetCursorPos({StartX + std::max((Width - ButtonsWidth) * 0.5f, 0.0f), RowY});
        ImGui::BeginDisabled(!Preview.IsReady());

        if (TransportButton("##ToStart", ETransportIcon::ToStart, false, "To Start (Home)")) {
            Preview.Seek(0.0);
        }

        ImGui::SameLine();

        if (TransportButton("##PreviousFrame", ETransportIcon::PreviousFrame, false, "Previous Frame (Left)")) {
            StepFrame(Preview, Sequence, -1);
        }

        ImGui::SameLine();

        if (TransportButton("##Reverse", Reverse ? ETransportIcon::Pause : ETransportIcon::PlayReverse, Reverse, Reverse ? "Pause (Space)" : "Play Reverse")) {
            if (Reverse) {
                Preview.Pause();
            } else {
                Preview.SetPlayRate(-std::max(std::abs(Preview.GetPlayRate()), 0.1f));
                Preview.Play();
            }
        }

        ImGui::SameLine();

        if (TransportButton("##Play", Forward ? ETransportIcon::Pause : ETransportIcon::Play, Forward, Forward ? "Pause (Space)" : "Play Forward")) {
            if (Forward) {
                Preview.Pause();
            } else {
                Preview.SetPlayRate(std::max(std::abs(Preview.GetPlayRate()), 0.1f));
                Preview.Play();
            }
        }

        ImGui::SameLine();

        if (TransportButton("##NextFrame", ETransportIcon::NextFrame, false, "Next Frame (Right)")) {
            StepFrame(Preview, Sequence, 1);
        }

        ImGui::SameLine();

        if (TransportButton("##ToEnd", ETransportIcon::ToEnd, false, "To End (End)")) {
            Preview.Seek(Sequence.GetPlayLength());
        }

        ImGui::EndDisabled();
        ImGui::SameLine();

        if (TransportButton("##Loop", ETransportIcon::Loop, Preview.IsLooping(), Preview.IsLooping() ? "Loop: On" : "Loop: Off")) {
            Preview.SetLooping(!Preview.IsLooping());
        }

        const float Speed{std::abs(Preview.GetPlayRate())};
        const Core::FString SpeedLabel{Speed < 1.0f ? std::format("x{:.2g}", Speed) : std::format("x{:.1f}", Speed)};
        const float ComboWidth{ImGui::CalcTextSize("x0.25").x + ImGui::GetFrameHeight() + Style.FramePadding.x * 2.0f};

        ImGui::SameLine();
        ImGui::SetCursorPos({StartX + std::max(Width - ComboWidth, 0.0f), RowY});
        ImGui::SetNextItemWidth(ComboWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.0f, 0.0f, 0.0f, 0.0f});

        if (ImGui::BeginCombo("##Speed", SpeedLabel.c_str())) {
            for (const float Candidate : std::array<float, 6>{0.1f, 0.25f, 0.5f, 1.0f, 1.5f, 2.0f}) {
                const Core::FString Label{Candidate < 1.0f ? std::format("x{:.2g}", Candidate) : std::format("x{:.1f}", Candidate)};
                const bool Selected{Candidate == Speed};

                if (ImGui::Selectable(Label.c_str(), Selected)) {
                    Preview.SetPlayRate(Preview.GetPlayRate() < 0.0f ? -Candidate : Candidate);
                }

                if (Selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::PopStyleColor(2);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Play Speed");
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
}
