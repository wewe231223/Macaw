/**
 * @file FStatisticsPanel.cpp
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FStatisticsPanel.h"
#include "Editor/EditorEngine.h"
#include "Core/Memory.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/PrimitiveMesh.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>
#include <format>

/**
 * @brief FStatisticsPanel의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FStatisticsPanel::FStatisticsPanel(UEditorEngine* Engine)
	: FEditorPanel{Engine, "Statistics", true} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FStatisticsPanel::~FStatisticsPanel() = default;

/**
 * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
 */
void FStatisticsPanel::DrawContent(float DeltaTime) {
    UEditorEngine* Engine{GetEditorEngine()};
    const FMemoryStats Memory{FMemory::GetStats()};
    const FRenderStats Render{Engine->GetRenderer()->GetStats()};
    const UWorld* World{Engine->GetWorld()};

    ImGui::TextUnformatted(std::format("Frame: {:.2f} ms", DeltaTime * 1000.0f).c_str());
    ImGui::TextUnformatted(std::format("Live UObjects: {}", Memory.mLiveObjects).c_str());
    ImGui::TextUnformatted(std::format("Created UObjects: {}", Memory.mCreatedObjects).c_str());
    ImGui::TextUnformatted(std::format("Actors: {}", World != nullptr && World->GetPersistentLevel() != nullptr ? World->GetPersistentLevel()->GetActors().size() : 0).c_str());
    ImGui::Separator();

    ImGui::TextUnformatted(std::format("UObject heap: {:.2f} KiB", Memory.mHeapBytes / 1024.0).c_str());
    ImGui::TextUnformatted(std::format("Peak UObject heap: {:.2f} KiB", Memory.mPeakHeapBytes / 1024.0).c_str());

    std::size_t GeometryBytes{};

    for (Uint32 Index{}; Index < static_cast<Uint32>(EPrimitiveType::Count); ++Index) {
        const FMeshData& Mesh{GetPrimitiveMesh(static_cast<EPrimitiveType>(Index))};
        GeometryBytes += Mesh.mVertices.capacity() * sizeof(FMeshVertex) + Mesh.mIndices.capacity() * sizeof(Uint32);
    }

    ImGui::TextUnformatted(std::format("Shared CPU meshes: {:.2f} KiB", GeometryBytes / 1024.0).c_str());
    ImGui::TextUnformatted(std::format("GPU buffers/target: {:.2f} MiB", Render.mGpuBytes / (1024.0 * 1024.0)).c_str());
    ImGui::Separator();

    ImGui::TextUnformatted(std::format("Visible primitives: {}", Render.mVisiblePrimitives).c_str());
    ImGui::TextUnformatted(std::format("Scene draw calls: {}", Render.mDrawCalls).c_str());
    ImGui::TextUnformatted(std::format("Object uploads: {}", Render.mTransformUpdates).c_str());
}
