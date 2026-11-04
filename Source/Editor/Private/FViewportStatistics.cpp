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
#include <cmath>
#include <format>
#include <iomanip>
#include <sstream>

namespace Editor {

    /**
     * @brief 통계 명령의 이름 정규화와 표시 문자열을 만드는 내부 도우미입니다.
     */
    namespace {
        /**
         * @brief 문자열의 각 문자를 소문자로 변환합니다.
         * @param[in] Text 소문자로 변환할 문자열입니다.
         * @return 소문자로 변환한 문자열 사본입니다.
         */
        Core::FString ToLower(Core::FString Text) {
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
        Core::FString FormatValue(double Value, Core::EStatUnit Unit, bool Average = false) {
            switch (Unit) {
            case Core::EStatUnit::Milliseconds:
                return std::format("{:.3f} ms", Value);
            case Core::EStatUnit::Bytes:
                return Value >= 1024.0 * 1024.0 ? std::format("{:.2f} MiB", Value / (1024.0 * 1024.0)) : std::format("{:.2f} KiB", Value / 1024.0);
            case Core::EStatUnit::Hertz:
                return std::format("{:.1f} FPS", Value);
            case Core::EStatUnit::Percent:
                return std::format("{:.2f}%", Value);
            default:
                return Average ? std::format("{:.2f}", Value) : std::format("{:.0f}", Value);
            }
        }

        void DrawHistory(const Core::FStatSnapshot& Stat) {
            const Core::TArray<double> History{Core::FStats::Get().GetHistory(Stat.GetGroup(), Stat.GetName())};
            const bool Bytes{Stat.GetUnit() == Core::EStatUnit::Bytes};
            const Core::FString Title{std::format("[{}] {}{}", Stat.GetGroup(), Stat.GetName(), Bytes ? " (MiB)" : "")};
            Core::TArray<float> Values{};
            Values.reserve(History.size());

            for (const double Value : History) {
                Values.push_back(static_cast<float>(Bytes ? Value / (1024.0 * 1024.0) : Value));
            }

            float Minimum{};
            float Maximum{};
            Core::FString Summary{"Waiting for a completed frame..."};

            if (!History.empty()) {
                const auto [Lowest, Highest]{std::minmax_element(History.begin(), History.end())};
                Minimum = static_cast<float>(Bytes ? *Lowest / (1024.0 * 1024.0) : *Lowest);
                Maximum = static_cast<float>(Bytes ? *Highest / (1024.0 * 1024.0) : *Highest);
                Summary = std::format("Now {}   |   Min {}   |   Max {}", FormatValue(History.back(), Stat.GetUnit()), FormatValue(*Lowest, Stat.GetUnit()), FormatValue(*Highest, Stat.GetUnit()));
            }

            const float Padding{Minimum == Maximum ? std::max(0.0001f, std::abs(Minimum) * 0.01f) : 0.0f};
            const ImVec2 Size{std::max(1.0f, ImGui::GetContentRegionAvail().x), ImGui::GetFrameHeight() * 4.0f};
            ImGui::PushID(Stat.GetGroup().c_str());
            ImGui::PushID(Stat.GetName().c_str());
            ImGui::TextUnformatted(Title.c_str());
            ImGui::PushStyleColor(ImGuiCol_PlotLines, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
            ImGui::PlotLines("##History", Values.data(), static_cast<int>(Values.size()), 0, nullptr, Minimum - Padding, Maximum + Padding, Size);
            ImGui::PopStyleColor();
            ImGui::TextUnformatted(Summary.c_str());
            ImGui::Spacing();
            ImGui::PopID();
            ImGui::PopID();
        }
    }

    Core::FString Editor::FStatGraphs::ExecuteCommand(std::string_view Arguments) {
        std::istringstream Input{Core::FString{Arguments}};
        Core::FString Group{};
        Core::FString Name{};
        Input >> Group >> std::ws;
        Group = ToLower(Group);

        if (Input.peek() == '"') {
            Input >> std::quoted(Name) >> std::ws;

            if (Input.fail() || !Input.eof() || Name.empty()) {
                return "Usage: stat graph [<group> [<stat name>] | all | clear | off]";
            }
        } else {
            std::getline(Input, Name);

            if (const std::size_t Last{Name.find_last_not_of(" \t\r\n\f\v")}; Last != Core::FString::npos) {
                Name.resize(Last + 1);
            }
        }

        if (Name.empty()) {
            if (Group.empty()) {
                if (mSelections.empty()) {
                    Show("fps");
                }

                mOpen = true;
                return "Statistics graphs opened.";
            }

            if (Group == "all") {
                Show({});
                return "Showing all statistics graphs.";
            }

            if (Group == "clear") {
                mSelections.clear();
                return "Statistics graph selection cleared.";
            }

            if (Group == "off") {
                mOpen = false;
                return "Statistics graphs closed.";
            }
        }

        if (Group == "shadows") {
            Group = "shadow";
        }

        const Core::TArray<Core::FString> Groups{Core::FStats::Get().GetGroups()};

        if (std::find(Groups.begin(), Groups.end(), Group) == Groups.end()) {
            return std::format("Unknown stat group '{}'. Use 'stat list' for available groups.", Group);
        }

        if (Name.empty()) {
            Show(Group);
            return std::format("Showing graphs for '{}'.", Group);
        }

        const Core::TArray<Core::FStatSnapshot> Snapshots{Core::FStats::Get().GetSnapshots(Group)};
        const Core::FString LowerName{ToLower(Name)};
        auto Found{std::find_if(Snapshots.begin(), Snapshots.end(), [&Name](const Core::FStatSnapshot& Stat) {
            return Stat.GetName() == Name;
        })};

        if (Found == Snapshots.end()) {
            Found = std::find_if(Snapshots.begin(), Snapshots.end(), [&LowerName](const Core::FStatSnapshot& Stat) {
                return ToLower(Stat.GetName()) == LowerName;
            });
        }

        if (Found == Snapshots.end()) {
            return std::format("Stat '{}' is unavailable in '{}'. Use the name shown in Statistics.", Name, Group);
        }

        Show(Group, Found->GetName());
        return std::format("Showing graph for [{}] {}.", Group, Found->GetName());
    }

    void Editor::FStatGraphs::Show(const Core::FString& Group, const Core::FString& Name) {
        if (Group.empty()) {
            mSelections.clear();
        }

        if (std::none_of(mSelections.begin(), mSelections.end(), [&Group, &Name](const Editor::FStatGraphs::FSelection& Selection) {
            return Selection.mGroup.empty() || (Selection.mGroup == Group && (Selection.mName.empty() || Selection.mName == Name));
        })) {
            mSelections.push_back({Group, Name});
        }

        mOpen = true;
    }

    void Editor::FStatGraphs::Draw() {
        if (!mOpen) {
            return;
        }

        ImGui::SetNextWindowSize({640.0f, 540.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Statistics Graphs", &mOpen)) {
            ImGui::TextUnformatted("Last 120 completed frames | Oldest to newest");
            ImGui::SameLine();

            if (ImGui::SmallButton("Clear graphs")) {
                mSelections.clear();
            }

            if (mSelections.empty()) {
                ImGui::TextUnformatted("Use 'stat graph <group>' or click Graph in the Statistics table.");
            } else {
                const Core::TArray<Core::FStatSnapshot> Snapshots{Core::FStats::Get().GetSnapshots()};
                Core::TArray<const Core::FStatSnapshot*> Visible{};

                for (const Core::FStatSnapshot& Stat : Snapshots) {
                    if (std::any_of(mSelections.begin(), mSelections.end(), [&Stat](const Editor::FStatGraphs::FSelection& Selection) {
                        return Selection.mGroup.empty() || (Selection.mGroup == Stat.GetGroup() && (Selection.mName.empty() || Selection.mName == Stat.GetName()));
                    })) {
                        Visible.push_back(&Stat);
                    }
                }

                if (Visible.empty()) {
                    ImGui::TextUnformatted("Waiting for a completed frame for the selected stats...");
                } else {
                    if (ImGui::BeginChild("HistoryList")) {
                        ImGuiListClipper Clipper{};
                        Clipper.Begin(static_cast<int>(Visible.size()));

                        while (Clipper.Step()) {
                            for (int Index{Clipper.DisplayStart}; Index < Clipper.DisplayEnd; ++Index) {
                                DrawHistory(*Visible[Index]);
                            }
                        }
                    }

                    ImGui::EndChild();
                }
            }
        }

        ImGui::End();
    }

    bool Editor::FViewportStatistics::ExecuteCommand(const Core::FString& Command, Core::FString& Output) {
        std::istringstream Input{Command};
        Core::FString Verb{};
        Core::FString Group{};
        Core::FString State{};
        Core::FString Extra{};
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

        const Core::TArray<Core::FString> Groups{Core::FStats::Get().GetGroups()};

        if ((Group == "list" || Group == "reset") && State.empty() && Extra.empty()) {
            if (Group == "reset") {
                Core::FStats::Get().Reset();
                Output = "Stat history reset.";
            } else {
                Output = "Stat groups:";

                for (const Core::FString& Name : Groups) {
                    Output += " " + Name;
                }
            }

            return true;
        }

        const auto Found{std::find_if(Groups.begin(), Groups.end(), [&Group](const Core::FString& Name) {
            return ToLower(Name) == Group;
        })};

        if (!Extra.empty() || (!State.empty() && State != "on" && State != "off") || (Group == "none" && !State.empty()) || (Found == Groups.end() && Group != "all" && Group != "none")) {
            Output = "Usage: stat <group>/all [on/off] | stat none | stat list | stat reset";
            return true;
        }

        bool Enable{};

        if (Group == "all" || Group == "none") {
            const bool AllEnabled{!Groups.empty() && std::all_of(Groups.begin(), Groups.end(), [this](const Core::FString& Name) {
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

    bool Editor::FViewportStatistics::IsEnabled(EGroup Group) const {
        const Core::Uint32 Mask{static_cast<Core::Uint32>(Group)};

        if (Mask == 0 || (Mask & static_cast<Core::Uint32>(EGroup::All)) != Mask) {
            return false;
        }

        const std::array<Core::FString, 10> Names{"fps", "scene", "memory", "render", "shadow", "unit", "game", "world", "script", "assets"};

        for (std::size_t Index{}; Index < Names.size(); ++Index) {
            if ((Mask & (1u << Index)) != 0 && !IsEnabled(Names[Index])) {
                return false;
            }
        }

        return true;
    }

    bool Editor::FViewportStatistics::IsEnabled(const Core::FString& Group) const {
        const bool Found{std::find(mEnabledGroups.begin(), mEnabledGroups.end(), Group) != mEnabledGroups.end()};
        return mAllGroups ? !Found : Found;
    }

    bool Editor::FViewportStatistics::IsVisible() const {
        return mAllGroups || !mEnabledGroups.empty();
    }

    void Editor::FViewportStatistics::DrawPanel(bool& Open, Editor::FStatGraphs& Graphs) {
        if (!Open) {
            return;
        }

        ImGui::SetNextWindowSize({920.0f, 500.0f}, ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Statistics", &Open)) {
            ImGui::Text("Completed frame: %llu | Average / Min / Max: last 120 frames", static_cast<unsigned long long>(Core::FStats::Get().GetFrameNumber()));

            if (ImGui::Button("Reset history")) {
                Core::FStats::Get().Reset();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputTextWithHint("##StatFilter", "Filter group or stat...", mFilter.data(), mFilter.size());

            const Core::FString Filter{ToLower(mFilter.data())};
            const auto Snapshots{Core::FStats::Get().GetSnapshots()};

            if (Snapshots.empty()) {
                ImGui::TextUnformatted("Waiting for a completed frame...");
            } else if (ImGui::BeginTable("StatValues", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, {0.0f, 0.0f})) {
                ImGui::TableSetupColumn("Group");
                ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Value");
                ImGui::TableSetupColumn("Average");
                ImGui::TableSetupColumn("Min");
                ImGui::TableSetupColumn("Max");
                ImGui::TableSetupColumn("Samples");
                ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const Core::FStatSnapshot& Stat : Snapshots) {
                    if (!Filter.empty() && ToLower(Stat.GetGroup() + " " + Stat.GetName()).find(Filter) == Core::FString::npos) {
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
                    ImGui::TableNextColumn();
                    ImGui::PushID(Stat.GetGroup().c_str());
                    ImGui::PushID(Stat.GetName().c_str());

                    if (ImGui::SmallButton("Graph")) {
                        Graphs.Show(Stat.GetGroup(), Stat.GetName());
                    }

                    ImGui::PopID();
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

        ImGui::End();
    }

    void Editor::FViewportStatistics::Draw(float, const Engine::UWorld*, const RenderCore::FRenderStats&, const Core::FVector2& Origin, const Core::FVector2& Size, ImDrawList* DrawList) const {
        if (!IsVisible() || Size.x < 1.0f || Size.y < 1.0f) {
            return;
        }

        Core::TArray<Core::FString> Lines{};

        for (const Core::FStatSnapshot& Stat : Core::FStats::Get().GetSnapshots()) {
            if (IsEnabled(Stat.GetGroup())) {
                const Core::FString Value{FormatValue(Stat.GetValue(), Stat.GetUnit())};
                Lines.push_back(Stat.GetUnit() == Core::EStatUnit::Milliseconds ? std::format("[{}] {}: {} (avg {} / max {})", Stat.GetGroup(), Stat.GetName(), Value, FormatValue(Stat.GetAverage(), Stat.GetUnit()), FormatValue(Stat.GetMaximum(), Stat.GetUnit())) : std::format("[{}] {}: {}", Stat.GetGroup(), Stat.GetName(), Value));
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

        for (const Core::FString& Line : Lines) {
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

        for (const Core::FString& Line : Lines) {
            DrawList->AddText(Position, IM_COL32(235, 240, 245, 255), Line.c_str());
            Position.y += LineHeight;
        }

        DrawList->PopClipRect();
    }

}
