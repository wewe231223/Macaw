/**
 * @file FViewportStatistics.cpp
 * @brief 통계 그룹 명령을 처리하고 패널과 뷰포트 오버레이에 측정값을 표시합니다.
 * @ingroup Editor
 */

#include "Editor/FViewportStatistics.h"
#include "Core/Math/FVector2.h"
#include "Core/Stats.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <format>
#include <sstream>

/**
 * @brief 통계 명령의 이름 정규화와 표시 문자열을 만드는 내부 도우미입니다.
 */
namespace {
    /**
     * @brief 문자열의 각 문자를 소문자로 변환합니다.
     * @param[in] Text 소문자로 변환할 문자열입니다.
     * @return 소문자로 변환한 문자열 사본입니다.
     */
    FString ToLower(FString Text) {
        std::transform(Text.begin(), Text.end(), Text.begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });
        return Text;
    }

    /**
     * @brief 통계 값을 단위와 평균 표시 여부에 맞는 문자열로 변환합니다.
     * @param[in] Value 기록할 유한한 통계 값입니다.
     * @param[in] Unit 기록하거나 표시할 값의 단위입니다.
     * @param[in] Average 누적 개수에도 소수점 평균 형식을 사용하려면 true입니다.
     * @return 단위 기호와 소수점 정밀도가 반영된 표시 문자열입니다.
     */
    FString FormatValue(double Value, EStatUnit Unit, bool Average = false) {
        switch (Unit) {
        case EStatUnit::Milliseconds:
            return std::format("{:.3f} ms", Value);
        case EStatUnit::Bytes:
            return Value >= 1024.0 * 1024.0 ? std::format("{:.2f} MiB", Value / (1024.0 * 1024.0)) : std::format("{:.2f} KiB", Value / 1024.0);
        case EStatUnit::Hertz:
            return std::format("{:.1f} FPS", Value);
        case EStatUnit::Percent:
            return std::format("{:.2f}%", Value);
        default:
            return Average ? std::format("{:.2f}", Value) : std::format("{:.0f}", Value);
        }
    }
}

bool FViewportStatistics::ExecuteCommand(const FString& Command, FString& Output) {
    std::istringstream Input{Command};
    FString Verb{};
    FString Group{};
    FString State{};
    FString Extra{};
    Input >> Verb >> Group >> State >> Extra;
    Verb = ToLower(Verb);
    Group = ToLower(Group);
    State = ToLower(State);
    if (Verb != "stat") {
        return false;
    }
    if (Group.empty()) {
        Group = "all";
    }
    if (Group == "shadows") {
        Group = "shadow";
    }
    const TArray<FString> Groups{FStats::Get().GetGroups()};
    if ((Group == "list" || Group == "reset") && State.empty() && Extra.empty()) {
        if (Group == "reset") {
            FStats::Get().Reset();
            Output = "Stat history reset.";
        } else {
            Output = "Stat groups:";
            for (const FString& Name : Groups) {
                Output += " " + Name;
            }
        }
        return true;
    }
    const auto Found{std::find_if(Groups.begin(), Groups.end(), [&Group](const FString& Name) {
        return ToLower(Name) == Group;
    })};
    if (!Extra.empty() || (!State.empty() && State != "on" && State != "off") || (Group == "none" && !State.empty()) || (Found == Groups.end() && Group != "all" && Group != "none")) {
        Output = "Usage: stat <group>/all [on/off] | stat none | stat list | stat reset";
        return true;
    }
    bool Enable{};
    if (Group == "all" || Group == "none") {
        const bool AllEnabled{!Groups.empty() && std::all_of(Groups.begin(), Groups.end(), [this](const FString& Name) {
            return IsEnabled(Name);
        })};
        Enable = Group != "none" && (State == "on" || (State.empty() && !AllEnabled));
        mAllGroups = Enable;
        mEnabledGroups.clear();
    } else {
        Group = *Found;
        Enable = State == "on" || (State.empty() && !IsEnabled(Group));
        std::erase(mEnabledGroups, Group);
        if (Enable != mAllGroups) {
            mEnabledGroups.push_back(Group);
        }
    }
    Output = std::format("stat {}: {}", Group, Enable ? "on" : "off");
    return true;
}

bool FViewportStatistics::IsEnabled(EGroup Group) const {
    const Uint32 Mask{static_cast<Uint32>(Group)};
    if (Mask == 0 || (Mask & static_cast<Uint32>(EGroup::All)) != Mask) {
        return false;
    }
    const std::array<FString, 10> Names{"fps", "scene", "memory", "render", "shadow", "unit", "game", "world", "script", "assets"};
    for (std::size_t Index{}; Index < Names.size(); ++Index) {
        if ((Mask & (1u << Index)) != 0 && !IsEnabled(Names[Index])) {
            return false;
        }
    }
    return true;
}

bool FViewportStatistics::IsEnabled(const FString& Group) const {
    const bool Found{std::find(mEnabledGroups.begin(), mEnabledGroups.end(), Group) != mEnabledGroups.end()};
    return mAllGroups ? !Found : Found;
}

bool FViewportStatistics::IsVisible() const {
    return mAllGroups || !mEnabledGroups.empty();
}

void FViewportStatistics::DrawPanel(bool& Open) {
    if (!Open) {
        return;
    }
    ImGui::SetNextWindowSize({920.0f, 500.0f}, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Statistics", &Open)) {
        ImGui::Text("Completed frame: %llu | Average / Min / Max: last 120 frames", static_cast<unsigned long long>(FStats::Get().GetFrameNumber()));
        if (ImGui::Button("Reset history")) {
            FStats::Get().Reset();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(260.0f);
        ImGui::InputTextWithHint("##StatFilter", "Filter group or stat...", mFilter.data(), mFilter.size());
        const FString Filter{ToLower(mFilter.data())};
        const auto Snapshots{FStats::Get().GetSnapshots()};
        if (Snapshots.empty()) {
            ImGui::TextUnformatted("Waiting for a completed frame...");
        } else if (ImGui::BeginTable("StatValues", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, {0.0f, 0.0f})) {
            ImGui::TableSetupColumn("Group");
            ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Value");
            ImGui::TableSetupColumn("Average");
            ImGui::TableSetupColumn("Min");
            ImGui::TableSetupColumn("Max");
            ImGui::TableSetupColumn("Samples");
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();
            for (const FStatSnapshot& Stat : Snapshots) {
                if (!Filter.empty() && ToLower(Stat.GetGroup() + " " + Stat.GetName()).find(Filter) == FString::npos) {
                    continue;
                }
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(Stat.GetGroup().c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(Stat.GetName().c_str());
                std::size_t ValueIndex{};
                for (const double Value : {Stat.GetValue(), Stat.GetAverage(), Stat.GetMinimum(), Stat.GetMaximum()}) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(FormatValue(Value, Stat.GetUnit(), ValueIndex++ == 1).c_str());
                }
                ImGui::TableNextColumn();
                ImGui::Text("%llu", static_cast<unsigned long long>(Stat.GetSamples()));
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

void FViewportStatistics::Draw(float, const UWorld*, const FRenderStats&, const FVector2& Origin, const FVector2& Size, ImDrawList* DrawList) const {
    if (!IsVisible() || Size.x < 1.0f || Size.y < 1.0f) {
        return;
    }
    TArray<FString> Lines{};
    for (const FStatSnapshot& Stat : FStats::Get().GetSnapshots()) {
        if (IsEnabled(Stat.GetGroup())) {
            const FString Value{FormatValue(Stat.GetValue(), Stat.GetUnit())};
            Lines.push_back(Stat.GetUnit() == EStatUnit::Milliseconds ? std::format("[{}] {}: {} (avg {} / max {})", Stat.GetGroup(), Stat.GetName(), Value, FormatValue(Stat.GetAverage(), Stat.GetUnit()), FormatValue(Stat.GetMaximum(), Stat.GetUnit())) : std::format("[{}] {}: {}", Stat.GetGroup(), Stat.GetName(), Value));
        }
    }
    if (Lines.empty()) {
        return;
    }
    const float LineHeight{ImGui::GetTextLineHeightWithSpacing()};
    const std::size_t MaximumLines{static_cast<std::size_t>(std::max(1.0f, (Size.y - 32.0f) / LineHeight))};
    if (Lines.size() > MaximumLines) {
        Lines.resize(MaximumLines);
        Lines.back() = "More stats: View > Statistics";
    }
    float Width{};
    for (const FString& Line : Lines) {
        Width = std::max(Width, ImGui::CalcTextSize(Line.c_str()).x);
    }
    const ImVec2 Minimum{std::max(Origin.x + 8.0f, Origin.x + Size.x - Width - 24.0f), Origin.y + 8.0f};
    const ImVec2 Maximum{Minimum.x + Width + 16.0f, Minimum.y + static_cast<float>(Lines.size()) * LineHeight + 16.0f};
    if (DrawList == nullptr) {
        DrawList = ImGui::GetWindowDrawList();
    }
    DrawList->PushClipRect({Origin.x, Origin.y}, {Origin.x + Size.x, Origin.y + Size.y}, true);
    DrawList->AddRectFilled(Minimum, Maximum, IM_COL32(15, 18, 22, 195), 4.0f);
    ImVec2 Position{Minimum.x + 8.0f, Minimum.y + 8.0f};
    for (const FString& Line : Lines) {
        DrawList->AddText(Position, IM_COL32(235, 240, 245, 255), Line.c_str());
        Position.y += LineHeight;
    }
    DrawList->PopClipRect();
}
