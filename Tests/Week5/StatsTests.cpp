#include "StatsTests.h"
#include "Core/Stats.h"
#include "Core/Math/FVector2.h"
#include "Editor/FViewportStatistics.h"
#include "Engine/Engine.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/AStaticMeshActor.h"
#include "RenderCore/FRenderStats.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
    void CheckStat(bool Condition, const char* Message) {
        if (!Condition) {
            throw std::runtime_error{Message};
        }
    }

    FStatSnapshot FindStat(const FString& Group, const FString& Name) {
        for (const FStatSnapshot& Snapshot : FStats::Get().GetSnapshots(Group)) {
            if (Snapshot.GetName() == Name) {
                return Snapshot;
            }
        }
        throw std::runtime_error{"Expected stat is missing"};
    }

    void TestStatAggregation() {
        FStats& Stats{FStats::Get()};
        Stats.Reset();
        FStat Counter{"stat_test", "Count"};
        FStat Gauge{"stat_test", "Gauge", EStatUnit::Bytes, EStatMode::Gauge};
        CheckStat(!Stats.EndFrame(), "EndFrame accepted an unmatched frame");
        CheckStat(Stats.BeginFrame() && !Stats.BeginFrame(), "Nested frame reset the current frame");
        Counter.Add(2.0);
        Counter.Add(3.0);
        Counter.Add(std::numeric_limits<double>::quiet_NaN());
        Counter.Add(std::numeric_limits<double>::infinity());
        Gauge.Set(4096.0);
        Stats.EndFrame();
        const FStatSnapshot First{FindStat("stat_test", "Count")};
        CheckStat(First.GetValue() == 5.0 && First.GetSamples() == 2 && First.GetAverage() == 5.0, "Counter accumulation or finite-value validation failed");
        CheckStat(Stats.GetFrameNumber() == 1 && First.GetFrameCount() == 1, "Completed frame count is incorrect");
        Stats.BeginFrame();
        Counter.Add(9.0);
        CheckStat(FindStat("stat_test", "Count").GetValue() == 5.0, "Readers observed an incomplete frame");
        Stats.EndFrame();
        const FStatSnapshot Second{FindStat("stat_test", "Count")};
        CheckStat(Second.GetValue() == 9.0 && Second.GetAverage() == 7.0 && Second.GetMinimum() == 5.0 && Second.GetMaximum() == 9.0, "Frame reset or rolling aggregation failed");
        CheckStat(First.GetValue() == 5.0, "A published snapshot changed after a later frame");
        CheckStat(FindStat("stat_test", "Gauge").GetValue() == 4096.0 && FindStat("stat_test", "Gauge").GetSamples() == 0, "Gauge did not retain its current value");
        {
            const FScopedStatFrame Frame{};
            const FScopedStatFrame NestedFrame{};
        }
        CheckStat(Stats.GetFrameNumber() == 3 && FindStat("stat_test", "Count").GetValue() == 0.0, "Idle or nested frames retained stale counters");
        bool Rejected{};
        try {
            FStat Duplicate{"stat_test", "Count"};
        } catch (const std::invalid_argument&) {
            Rejected = true;
        }
        CheckStat(Rejected, "Duplicate stat registration was accepted");
        Rejected = false;
        try {
            FStat Invalid{"invalid group", "Count"};
        } catch (const std::invalid_argument&) {
            Rejected = true;
        }
        CheckStat(Rejected, "An unreachable group token was registered");
        Stats.Reset();
        for (Uint32 Index{1}; Index <= 125; ++Index) {
            const FScopedStatFrame Frame{};
            Counter.Add(Index);
        }
        const FStatSnapshot Window{FindStat("stat_test", "Count")};
        CheckStat(Window.GetFrameCount() == 120 && Window.GetMinimum() == 6.0 && Window.GetMaximum() == 125.0 && std::abs(Window.GetAverage() - 65.5) < 0.000001, "The rolling history did not evict old frames");
        Stats.Reset();
        CheckStat(Stats.GetFrameNumber() == 0 && Stats.GetSnapshots().empty(), "Reset retained published history");
        {
            const FScopedStatFrame Frame{};
            Gauge.Set(8192.0);
        }
        Stats.Reset();
        {
            const FScopedStatFrame Frame{};
        }
        CheckStat(FindStat("stat_test", "Gauge").GetValue() == 8192.0, "Reset history destroyed a retained gauge value");
    }

    void TestStatThreadsAndTimers() {
        FStats& Stats{FStats::Get()};
        FStat Counter{"stat_test", "Thread count"};
        FStat Timer{"stat_test", "Scope time", EStatUnit::Milliseconds};
        {
            const FScopedStatFrame Frame{};
            std::array<std::thread, 4> Workers{};
            for (std::thread& Worker : Workers) {
                Worker = std::thread{[&Counter]() {
                    for (Uint32 Index{}; Index < 1000; ++Index) {
                        Counter.Add();
                    }
                }};
            }
            for (std::thread& Worker : Workers) {
                Worker.join();
            }
            try {
                const FScopedStatTimer Outer{Timer};
                const FScopedStatTimer Inner{Timer};
                throw std::runtime_error{"Unwind timers"};
            } catch (const std::runtime_error&) {
            }
        }
        CheckStat(FindStat("stat_test", "Thread count").GetValue() == 4000.0, "Concurrent writers lost samples");
        const FStatSnapshot Timing{FindStat("stat_test", "Scope time")};
        CheckStat(Timing.GetSamples() == 2 && Timing.GetValue() >= 0.0, "Scope timers lost nested or unwound samples");
        FStopwatch Watch{};
        CheckStat(Watch.GetElapsedMilliseconds() >= 0.0, "Stopwatch returned negative elapsed time");
        Watch.Reset();
        CheckStat(Watch.GetElapsedMilliseconds() >= 0.0, "Stopwatch reset failed");
        {
            FStat Temporary{"temporary_stat_group", "Temporary"};
            const FScopedStatFrame Frame{};
            Temporary.Add();
        }
        const auto Groups{Stats.GetGroups()};
        CheckStat(std::find(Groups.begin(), Groups.end(), "temporary_stat_group") == Groups.end() && Stats.GetSnapshots("temporary_stat_group").empty(), "Destroyed stats left dangling registry entries");
    }

    void TestEngineStats() {
        UEngine Engine{};
        CheckStat(Engine.Initialize(), "Stats engine failed to initialize");
        AStaticMeshActor* Actor{Engine.GetWorld()->SpawnPrimitive(EPrimitiveType::Cube)};
        CheckStat(Actor != nullptr, "Stats actor failed to spawn");
        Actor->SetEditorTickable(true);
        Actor->GetStaticMeshComponent()->SetEditorTickable(true);
        Engine.CreateWorld(EWorldType::EditorPreview);
        FStats::Get().Reset();
        {
            const FScopedStatFrame Frame{};
            Engine.Tick(0.016f);
            Engine.SynchronizeWorld();
        }
        CheckStat(FindStat("scene", "Worlds (engine)").GetValue() == 2.0 && FindStat("scene", "Actors (engine worlds)").GetValue() == 1.0, "Engine scene stats did not aggregate multiple worlds");
        CheckStat(FindStat("game", "World ticks").GetValue() == 2.0 && FindStat("game", "Actor ticks").GetValue() == 1.0 && FindStat("game", "Component ticks").GetValue() == 1.0, "Actual world, actor or component ticks were not measured");
        {
            const FScopedStatFrame Frame{};
            Engine.Tick(0.016f, false);
        }
        CheckStat(FindStat("game", "World ticks").GetValue() == 0.0 && FindStat("scene", "Actors (engine worlds)").GetValue() == 1.0, "Skipping editor ticks lost scene gauges or retained tick counts");
        Engine.Shutdown();
    }

    void TestStatCommandsAndPanel() {
        FViewportStatistics Statistics{};
        FViewportStatistics OtherViewport{};
        FString Output{};
        CheckStat(!Statistics.ExecuteCommand("help", Output) && !Statistics.IsVisible(), "Stats consumed an unrelated command");
        CheckStat(Statistics.ExecuteCommand("STAT FPS ON", Output) && Statistics.IsEnabled(FViewportStatistics::EGroup::Fps) && !OtherViewport.IsVisible(), "Stat command case handling or viewport isolation failed");
        Statistics.ExecuteCommand("stat all on", Output);
        FStat Dynamic{"dynamic_stat_group", "Dynamic"};
        CheckStat(Statistics.IsEnabled("dynamic_stat_group"), "Stat all did not include later registrations");
        Statistics.ExecuteCommand("stat dynamic_stat_group off", Output);
        CheckStat(!Statistics.IsEnabled("dynamic_stat_group") && Statistics.IsEnabled("fps"), "Disabling a group changed other groups");
        Statistics.ExecuteCommand("stat list", Output);
        CheckStat(Output.find("dynamic_stat_group") != FString::npos, "Registered groups were not discoverable");
        Statistics.ExecuteCommand("stat fps off extra", Output);
        CheckStat(Statistics.IsEnabled("fps") && Output.starts_with("Usage:"), "Invalid stat arguments mutated visibility");
        Statistics.ExecuteCommand("stat reset", Output);
        CheckStat(FStats::Get().GetSnapshots().empty(), "Stat reset did not clear history");
        {
            const FScopedStatFrame Frame{};
            Dynamic.Add(3.0);
        }
        ImGui::CreateContext();
        ImGuiIO& Input{ImGui::GetIO()};
        Input.IniFilename = nullptr;
        Input.DisplaySize = {1280.0f, 800.0f};
        Input.DeltaTime = 1.0f / 60.0f;
        Input.Fonts->AddFontDefault();
        Input.Fonts->Build();
        ImGui::NewFrame();
        bool Open{true};
        Statistics.DrawPanel(Open);
        ImGui::Begin("Stats overlay test");
        Statistics.Draw(0.0f, nullptr, {}, {0.0f, 0.0f}, {300.0f, 90.0f});
        ImGui::End();
        ImGui::Render();
        ImGui::DestroyContext();
        Statistics.ExecuteCommand("stat none", Output);
        CheckStat(!Statistics.IsVisible(), "Stat none retained overlays");
    }
}

void TestStats() {
    TestStatAggregation();
    TestStatThreadsAndTimers();
    TestEngineStats();
    TestStatCommandsAndPanel();
    FStats::Get().Reset();
}
