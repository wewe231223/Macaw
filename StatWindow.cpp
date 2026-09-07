#include "PCH.h"
#include "Render/Stats/StatWindow.h"

#include "Core/Memory/Memory.h"
#include "Core/Base/UObjectSystem.h"
#include "Scene/UWorld.h"
#include "ImGui/imgui.h"

void DrawStatWindow(const UWorld& World)
{
	if (!ImGui::Begin("Stats"))
	{
		ImGui::End();
		return;
	}

	const uint32 ObjectCount = UObjectSystem::GetObjectCount();


	ImGui::TextUnformatted("Object System");
	ImGui::Separator();

	ImGui::Text("Registered UObjects: %u", ObjectCount);

	ImGui::Text("Actors : %llu", static_cast<unsigned long long>(World.GetActors().size()));

	ImGui::Spacing();

	const Memory::FMemoryStats Stats = Memory::GetStats();

	const double AllocatedKiB = static_cast<double>(Stats.AllocatedBytes) / 1024.0;

	const double PeakKiB = static_cast<double>(Stats.PeakAllocatedBytes) / 1024.0;

	ImGui::TextUnformatted("Heap Memory");
	ImGui::Separator();

	ImGui::Text("Allocated : %llu bytes (%.2f Kib)", static_cast<unsigned long long>(Stats.AllocatedBytes), AllocatedKiB);

	ImGui::Text("Active allocations: %llu", static_cast<unsigned long long>(Stats.ActiveAllocationCount));

	ImGui::Text("Total allcations: %llu", static_cast<unsigned long long>(Stats.TotalAllocationCount));

	//ImGui::Text("Peak Allocated: %llu bytes (%.2f Kib)", static_cast<unsigned long long>(Stats.PeakAllocatedBytes), PeakKiB);

	float UsageRatio = 0.0f;

	if (Stats.PeakAllocatedBytes > 0) {
		UsageRatio = static_cast<float>(Stats.AllocatedBytes) / static_cast<float>(Stats.PeakAllocatedBytes);
	}

	if (UsageRatio > 1.0f)
	{
		UsageRatio = 1.0f;
	}

	ImGui::ProgressBar(UsageRatio, ImVec2(-1.0f, 0.0f), "Current / Peak");

	ImGui::Separator();

	ImGui::TextDisabled("Tracked allocations made through Memory::Allocate()");

	ImGui::End();
}