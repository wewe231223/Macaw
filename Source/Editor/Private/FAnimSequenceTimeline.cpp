/**
 * @file FAnimSequenceTimeline.cpp
 * @brief 시퀀스의 프레임 눈금, 탐색 구간과 재생 조작 UI를 관리합니다.
 * @ingroup Editor
 */

#include "FAnimSequenceTimeline.h"

#include "Viewer/FAnimationViewerContent.h"
#include "Asset/Animation/UAnimSequence.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>
#include <imgui.h>
#include <imgui_neo_sequencer.h>

using namespace Editor;

/**
 * @brief 시퀀스의 프레임 눈금, 탐색 구간과 재생 조작 UI를 관리하는 파일 내부 보조 구현입니다.
 */
namespace {

    /**
     * @brief 애니메이션 재생 조작 버튼에 그릴 아이콘 종류입니다.
     * @ingroup Editor
     */
    enum class ETransportIcon {

        /**
         * @brief 시퀀스 시작으로 이동합니다.
         */
        ToStart,

        /**
         * @brief 이전 프레임으로 이동합니다.
         */
        PreviousFrame,

        /**
         * @brief 역방향 재생을 시작합니다.
         */
        PlayReverse,

        /**
         * @brief 현재 시각에서 일시 정지합니다.
         */
        Pause,

        /**
         * @brief 정방향 재생을 시작합니다.
         */
        Play,

        /**
         * @brief 다음 프레임으로 이동합니다.
         */
        NextFrame,

        /**
         * @brief 시퀀스 끝으로 이동합니다.
         */
        ToEnd,

        /**
         * @brief 반복 재생을 전환합니다.
         */
        Loop
    };

    /**
     * @brief 방향과 채움 방식에 맞는 재생 아이콘 삼각형을 그립니다.
     * @param DrawList 도형을 기록할 ImGui 그리기 목록입니다.
     * @param Center 아이콘 또는 카메라 중심 좌표입니다.
     * @param Width 출력 또는 UI 영역의 너비(픽셀)입니다.
     * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
     * @param Right 삼각형이 오른쪽을 향하면 true입니다.
     * @param Filled 삼각형 내부를 채울지 여부입니다.
     * @param Color 도형 또는 텍스트의 표시 색상입니다.
     */
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

    /**
     * @brief 재생 아이콘의 수직 막대를 그립니다.
     * @param DrawList 도형을 기록할 ImGui 그리기 목록입니다.
     * @param X 에디터 배치의 X 좌표입니다.
     * @param Y 에디터 배치의 Y 좌표입니다.
     * @param Width 출력 또는 UI 영역의 너비(픽셀)입니다.
     * @param Height 출력 또는 UI 영역의 높이(픽셀)입니다.
     * @param Color 도형 또는 텍스트의 표시 색상입니다.
     */
    void DrawBar(ImDrawList* DrawList, float X, float Y, float Width, float Height, ImU32 Color) {
        DrawList->AddRectFilled({X - Width * 0.5f, Y - Height * 0.5f}, {X + Width * 0.5f, Y + Height * 0.5f}, Color);
    }

    /**
     * @brief 재생 조작 종류에 대응하는 벡터 도형 아이콘을 그립니다.
     * @param DrawList 도형을 기록할 ImGui 그리기 목록입니다.
     * @param Icon 리소스가 없을 때 사용할 대체 아이콘 정보입니다.
     * @param Center 아이콘 또는 카메라 중심 좌표입니다.
     * @param Size 출력 또는 표시 영역 크기(픽셀)입니다.
     * @param Color 도형 또는 텍스트의 표시 색상입니다.
     */
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

    /**
     * @brief 재생 조작 아이콘 버튼과 툴팁을 표시합니다.
     * @param Id 조회 또는 변경할 항목 ID입니다.
     * @param Icon 리소스가 없을 때 사용할 대체 아이콘 정보입니다.
     * @param Active 현재 조작 상태를 강조 표시할지 여부입니다.
     * @param Tooltip 버튼에 표시할 마우스 오버 설명입니다.
     * @return 버튼을 눌렀으면 true입니다.
     */
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

void FAnimSequenceTimeline::Draw(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, float Height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(24, 24, 24, 255));

    if (ImGui::BeginChild("##AnimationTimeline", {0.0f, Height}, ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        const float BarHeight{ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y + ImGui::GetStyle().ItemSpacing.y};
        DrawTimeArea(Preview, Sequence, std::max(ImGui::GetContentRegionAvail().y - BarHeight, 1.0f));
        DrawTransportBar(Preview, Sequence);
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void FAnimSequenceTimeline::DrawTimeArea(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, float Height) {
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

void FAnimSequenceTimeline::StepFrame(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence, Core::Int32 Direction) {
    const double Rate{Sequence.GetDataModel().GetFrameRate().AsDecimal()};
    const double Frame{Preview.GetPosition() * Rate};
    const double Next{Direction < 0 ? std::ceil(Frame - 0.00001) - 1.0 : std::floor(Frame + 0.00001) + 1.0};

    Preview.Seek(Next / Rate);
}

void FAnimSequenceTimeline::DrawTransportBar(Viewer::FAnimationViewerContent& Preview, const Asset::UAnimSequence& Sequence) {
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
