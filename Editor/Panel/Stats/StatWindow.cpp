#include "pch.h"
#include "Editor/Panel/Stats/StatWindow.h"

#include "Core/Memory/Memory.h"
#include "Core/Base/UObjectSystem.h"
#include "World/UWorld.h"
#include "ImGui/imgui.h"
#include "World/AActor.h"

void DrawStatContents(const UWorld& World, FStatDisplayFlags StatFlags) {
    const float FPS{ImGui::GetIO().Framerate};
    const float FrameTimeMs{FPS > 0.0f ? 1000.0f / FPS : 0.0f};

    //if(Reader.Peek() == EStatDisplayMode::Fps)
    if (StatFlags.mBShowFps) {
        ImGui::Text("FPS: %.1f (%.2f ms)", FPS, FrameTimeMs);

        ImGui::Spacing();
    }

    if (StatFlags.mBObjectSystem) {
        const Uint32 ObjectCount{UObjectSystem::GetObjectCount()};

        ImGui::TextUnformatted("Object System");
        ImGui::Separator();

        ImGui::Text("Registered UObjects: %u", ObjectCount);
        ImGui::Text("Actors : %llu", static_cast<unsigned long long>(World.GetActors().size()));

        ImGui::Spacing();
    }

    if (StatFlags.mBShowMemory) {
        const Memory::FMemoryStats Stats{Memory::GetStats()};

        const double AllocatedKiB{static_cast<double>(Stats.mAllocatedBytes) / 1024.0};
        const double PeakKiB{static_cast<double>(Stats.mPeakAllocatedBytes) / 1024.0};

        ImGui::TextUnformatted("Heap Memory");
        ImGui::Separator();

        ImGui::Text("Allocated : %llu bytes (%.2f KiB)", static_cast<unsigned long long>(Stats.mAllocatedBytes), AllocatedKiB);
        ImGui::Text("Peak Allocated: %llu bytes (%.2f KiB)", static_cast<unsigned long long>(Stats.mPeakAllocatedBytes), PeakKiB);
        ImGui::Text("Active allocations: %llu", static_cast<unsigned long long>(Stats.mActiveAllocationCount));
        ImGui::Text("Total allocation calls: %llu", static_cast<unsigned long long>(Stats.mTotalAllocationCount));
        ImGui::Text("Total deallocation calls: %llu", static_cast<unsigned long long>(Stats.mTotalDeallocationCount));

        float UsageRatio{0.0f};
        if (Stats.mPeakAllocatedBytes > 0) {
            UsageRatio = static_cast<float>(Stats.mAllocatedBytes) / static_cast<float>(Stats.mPeakAllocatedBytes);
        }
        if (UsageRatio > 1.0f)
            UsageRatio = 1.0f;

        ImGui::ProgressBar(UsageRatio, ImVec2(-1.0f, 0.0f), "Current / Peak");

        ImGui::Spacing();
        ImGui::TextUnformatted("Memory Usage by Tag");
        ImGui::Separator();

        if (ImGui::BeginTable("TagMemoryStatsTable", 4, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("Tag");
            ImGui::TableSetupColumn("Allocated");
            ImGui::TableSetupColumn("Active Count");
            ImGui::TableSetupColumn("Share");
            ImGui::TableHeadersRow();

            for (std::size_t I{0}; I < static_cast<std::size_t>(Memory::EMemoryTag::Count); ++I) {
                const Memory::EMemoryTag Tag{static_cast<Memory::EMemoryTag>(I)};
                const auto& TagStat{Stats.mTagStats[I]};
                const double TagKiB{static_cast<double>(TagStat.mAllocatedBytes) / 1024.0};

                const float SharePercent{(Stats.mAllocatedBytes > 0) ? (static_cast<float>(TagStat.mAllocatedBytes) / static_cast<float>(Stats.mAllocatedBytes)) * 100.0f : 0.0f};

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(Memory::GetMemoryTagName(Tag));

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f KiB (%llu B)", TagKiB, static_cast<unsigned long long>(TagStat.mAllocatedBytes));

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%llu", static_cast<unsigned long long>(TagStat.mActiveAllocationCount));

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.1f%%", SharePercent);
            }
        }

        ImGui::EndTable();

        ImGui::Separator();
        ImGui::TextDisabled("Tracked allocations made through Memory::Allocate()");
    }
}
