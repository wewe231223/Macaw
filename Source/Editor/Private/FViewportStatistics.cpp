/**
 * @file FViewportStatistics.cpp
 * @brief 프레임 시간, 객체, 메모리와 렌더링 통계를 표시합니다.
 * @ingroup Editor
 */

#include "Editor/FViewportStatistics.h"
#include "Core/Math/FVector2.h"
#include "Core/Memory.h"
#include "Engine/UWorld.h"
#include "Engine/ULevel.h"
#include "Engine/PrimitiveMesh.h"
#include "RenderCore/FRenderStats.h"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <sstream>

/**
 * @brief 뷰포트별 통계 표시 상태를 명령에 따라 변경합니다.
 * @param[in] Command 해석할 stat 명령과 인수입니다.
 * @param[out] Output 처리 결과 또는 사용법 진단을 받을 문자열입니다.
 * @return stat 명령을 처리했으면 true이며 다른 종류의 명령이면 false입니다.
 */
bool FViewportStatistics::ExecuteCommand(const FString& Command, FString& Output) {
    std::istringstream Input{Command};
    FString Verb{};
    FString Group{};
    FString State{};
    FString Extra{};
    Input >> Verb >> Group >> State >> Extra;
    for (FString* Token : {&Verb, &Group, &State}) {
        std::transform(Token->begin(), Token->end(), Token->begin(), [](unsigned char Character) {
            return static_cast<char>(std::tolower(Character));
        });
    }
    if (Verb != "stat") {
        return false;
    }

    if (Group.empty()) {
        Group = "all";
    }
    Uint8 Mask{};
    if (Group == "fps") {
        Mask = static_cast<Uint8>(EGroup::Fps);
    } else if (Group == "scene") {
        Mask = static_cast<Uint8>(EGroup::Scene);
    } else if (Group == "memory") {
        Mask = static_cast<Uint8>(EGroup::Memory);
    } else if (Group == "render") {
        Mask = static_cast<Uint8>(EGroup::Render);
    } else if (Group == "all" || Group == "none") {
        Mask = static_cast<Uint8>(EGroup::All);
    }

    if (Mask == 0 || !Extra.empty() || (!State.empty() && State != "on" && State != "off") || (Group == "none" && !State.empty())) {
        Output = "Usage: stat fps/scene/memory/render/all [on/off] | stat none";
        return true;
    }

    const bool Enable{Group != "none" && (State == "on" || (State.empty() && (mEnabledGroups & Mask) != Mask))};
    mEnabledGroups = Enable ? static_cast<Uint8>(mEnabledGroups | Mask) : static_cast<Uint8>(mEnabledGroups & ~Mask);
    Output = std::format("stat {}: {}", Group, Enable ? "on" : "off");
    return true;
}

/**
 * @brief 선택한 통계 그룹의 표시 여부를 반환합니다.
 * @param[in] Group 표시 여부를 조회할 통계 그룹 비트 조합입니다.
 * @return 모든 요청 그룹이 유효하고 표시 중이면 true입니다.
 */
bool FViewportStatistics::IsEnabled(EGroup Group) const {
    const Uint8 Mask{static_cast<Uint8>(Group)};
    return Mask != 0 && (Mask & static_cast<Uint8>(EGroup::All)) == Mask && (mEnabledGroups & Mask) == Mask;
}

bool FViewportStatistics::IsVisible() const {
    return mEnabledGroups != 0;
}

/**
 * @brief 뷰포트 이미지 위에 입력을 받지 않는 통계 내용을 그립니다.
 */
void FViewportStatistics::Draw(float DeltaTime, const UWorld* World, const FRenderStats& Render, const FVector2& Origin, const FVector2& Size, ImDrawList* DrawList) const {
    if (!IsVisible() || Size.x < 1.0f || Size.y < 1.0f) {
        return;
    }

    TArray<FString> Lines{};
    const FMemoryStats Memory{FMemory::GetStats()};
    if (IsEnabled(EGroup::Fps)) {
        const float FrameTime{std::isfinite(DeltaTime) && DeltaTime > 0.0f ? DeltaTime : 0.0f};
        Lines.push_back(std::format("FPS: {:.1f} | Frame: {:.2f} ms", FrameTime > 0.0f ? 1.0f / FrameTime : 0.0f, FrameTime * 1000.0f));
    }
    if (IsEnabled(EGroup::Scene)) {
        Lines.push_back(std::format("Actors: {}", World != nullptr && World->GetPersistentLevel() != nullptr ? World->GetPersistentLevel()->GetActors().size() : 0));
        Lines.push_back(std::format("Live UObjects (global): {}", Memory.mLiveObjects));
        Lines.push_back(std::format("Created UObjects (global): {}", Memory.mCreatedObjects));
    }
    if (IsEnabled(EGroup::Memory)) {
        std::size_t GeometryBytes{};
        for (Uint32 Index{}; Index < static_cast<Uint32>(EPrimitiveType::Count); ++Index) {
            const FMeshData& Mesh{GetPrimitiveMesh(static_cast<EPrimitiveType>(Index))};
            GeometryBytes += Mesh.GetAllocatedBytes();
        }
        Lines.push_back(std::format("UObject heap (global): {:.2f} KiB", Memory.mHeapBytes / 1024.0));
        Lines.push_back(std::format("Peak UObject heap: {:.2f} KiB", Memory.mPeakHeapBytes / 1024.0));
        Lines.push_back(std::format("Shared CPU meshes: {:.2f} KiB", GeometryBytes / 1024.0));
        Lines.push_back(std::format("GPU buffers/target: {:.2f} MiB", Render.mGpuBytes / (1024.0 * 1024.0)));
    }
    if (IsEnabled(EGroup::Render)) {
        Lines.push_back(std::format("Visible primitives: {}", Render.mVisiblePrimitives));
        Lines.push_back(std::format("Scene draw calls: {}", Render.mDrawCalls));
        Lines.push_back(std::format("Object uploads: {}", Render.mTransformUpdates));
    }

    float Width{};
    for (const FString& Line : Lines) {
        Width = std::max(Width, ImGui::CalcTextSize(Line.c_str()).x);
    }
    const float LineHeight{ImGui::GetTextLineHeightWithSpacing()};
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
