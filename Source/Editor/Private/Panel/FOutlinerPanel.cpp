/**
 * @file FOutlinerPanel.cpp
 * @brief 월드의 액터 목록과 선택 상태를 표시합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FOutlinerPanel.h"
#include "Editor/FEditorContext.h"
#include "EditorStyle.h"
#include "Engine/AActor.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/UActorComponent.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/EPrimitiveType.h"
#include "Panel/ObjectRow.h"

#include <imgui.h>
#include <algorithm>
#include <iterator>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 객체·클래스 이름이 Outliner 필터에 맞는지 검사합니다.
         * @param[in] Filter 객체·클래스 이름을 검사할 필터입니다.
         * @param[in] Object 조회하거나 호출할 엔진·COM 객체입니다.
         * @return 객체·클래스 이름이 필터를 통과하면 true입니다.
         */
        bool MatchesFilter(const ImGuiTextFilter& Filter, const CoreUObject::UObject* Object) {
            return Filter.PassFilter(Object->GetName().c_str()) || Filter.PassFilter(ObjectRow::GetTypeLabel(Object).c_str());
        }

        /**
         * @brief 액터 표시 상태 버튼을 그리고 클릭 여부를 반환합니다.
         * @param[in] ActorVisible 액터의 현재 표시 상태입니다.
         * @return 표시 전환 버튼을 클릭했으면 true입니다.
         */
        bool DrawVisibilityButton(bool ActorVisible) {
            const float Size{ImGui::GetFontSize()};
            const bool Pressed{ImGui::InvisibleButton("##Visibility", {Size, Size})};
            const ImU32 Color{ImGui::GetColorU32(ActorVisible || ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled)};

            EditorStyle::DrawIcon(ActorVisible ? EditorStyle::EIcon::Visible : EditorStyle::EIcon::Hidden, ImGui::GetItemRectMin(), Size, Color);
            ImGui::SetItemTooltip("%s", ActorVisible ? "숨기기" : "표시");

            return Pressed;
        }
    }

    /**
     * @brief FOutlinerPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FOutlinerPanel::FOutlinerPanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "World Outliner"} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FOutlinerPanel::~FOutlinerPanel() = default;

    /**
     * @brief 패널 고유의 ImGui 위젯과 내용을 그립니다.
     */
    void Editor::FOutlinerPanel::DrawContent(float) {
        Editor::FEditorContext* Context{GetEditorContext()};

        ImGui::BeginDisabled(!Context->CanEditWorld());

        if (ImGui::Button("+ 액터")) {
            ImGui::OpenPopup("AddActor");
        }

        if (ImGui::BeginPopup("AddActor")) {
            if (ImGui::MenuItem("Plane")) {
                Context->SpawnPrimitive(Engine::EPrimitiveType::Plane);
            }

            if (ImGui::MenuItem("Cube")) {
                Context->SpawnPrimitive(Engine::EPrimitiveType::Cube);
            }

            if (ImGui::MenuItem("Sphere")) {
                Context->SpawnPrimitive(Engine::EPrimitiveType::Sphere);
            }

            if (ImGui::MenuItem("Camera")) {
                Context->SpawnCamera();
            }

            if (ImGui::MenuItem("Text")) {
                Context->SpawnText();
            }

            if (ImGui::MenuItem("Billboard")) {
                Context->SpawnBillboard();
            }

            if (ImGui::BeginMenu("Editor Icons")) {
                if (ImGui::MenuItem("Spot Light Icon")) {
                    Context->SpawnBillboard("Editor/Icon/SpotLight_64x.png");
                }

                if (ImGui::MenuItem("Point Light Icon")) {
                    Context->SpawnBillboard("Editor/Icon/PointLight_64x.png");
                }

                if (ImGui::MenuItem("Pawn Icon")) {
                    Context->SpawnBillboard("Editor/Icon/Pawn_64x.png");
                }

                ImGui::EndMenu();
            }

            ImGui::EndPopup();
        }

        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputTextWithHint("##Search", "검색", mSearch.data(), mSearch.size());

        const ImGuiTextFilter Filter{mSearch.data()};
        const auto Actors{Context->GetActorHandles()};
        const Core::TArray<CoreUObject::FObjectHandle> Selection{Context->GetSelectedActors()};
        Core::TArray<CoreUObject::FObjectHandle> Visible{};

        for (CoreUObject::FObjectHandle Handle : Actors) {
            const Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(Context->ResolveObject(Handle))};

            if (Actor != nullptr && MatchesFilter(Filter, Actor)) {
                Visible.push_back(Handle);
            }
        }

        const bool CanEdit{Context->CanEditWorld()};
        bool DuplicateRequested{};
        bool DeleteRequested{};
        constexpr ImGuiTableFlags TableFlags{ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY};

        if (ImGui::BeginTable("Actors", 3, TableFlags, {0.0f, -ImGui::GetTextLineHeightWithSpacing()})) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("##Visibility", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoHide, ImGui::GetFontSize());
            ImGui::TableSetupColumn("Item Label", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("DirectionalLightActor").x);
            ImGui::TableHeadersRow();

            for (CoreUObject::FObjectHandle Handle : Visible) {
                Engine::AActor* Actor{CoreUObject::Cast<Engine::AActor>(Context->ResolveObject(Handle))};

                if (Actor == nullptr) {
                    continue;
                }

                ImGui::PushID(Actor);

                const bool Selected{std::find(Selection.begin(), Selection.end(), Handle) != Selection.end()};

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                // 행 전체를 덮는 라벨보다 먼저 제출해야 버튼이 클릭을 먼저 받습니다.
                ImGui::BeginDisabled(!CanEdit);

                if (DrawVisibilityButton(Actor->IsVisible())) {
                    const bool ActorVisible{!Actor->IsVisible()};

                    // 선택된 행의 버튼은 언리얼처럼 선택된 액터 전체에 적용합니다.
                    if (Selected) {
                        for (CoreUObject::FObjectHandle SelectedHandle : Selection) {
                            if (auto* SelectedActor{CoreUObject::Cast<Engine::AActor>(Context->ResolveObject(SelectedHandle))}) {
                                SelectedActor->SetVisible(ActorVisible);
                            }
                        }
                    } else {
                        Actor->SetVisible(ActorVisible);
                    }

                    Context->MarkSceneDirty();
                }

                ImGui::EndDisabled();

                const float RowAlpha{ImGui::GetStyle().Alpha * (Actor->IsVisible() ? 1.0f : 0.5f)};

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, RowAlpha);
                ObjectRow::DrawLabel(Actor, Selected);
                ImGui::PopStyleVar();

                if (ImGui::IsItemClicked()) {
                    const auto Anchor{Selection.empty() ? Visible.end() : std::find(Visible.begin(), Visible.end(), Selection.back())};

                    if (ImGui::GetIO().KeyCtrl) {
                        Context->ToggleActorSelection(Handle);
                    } else if (ImGui::GetIO().KeyShift && Anchor != Visible.end()) {
                        // 클릭한 액터가 마지막에 오도록 앵커 방향으로 범위를 수집합니다.
                        const auto Clicked{std::find(Visible.begin(), Visible.end(), Handle)};
                        Core::TArray<CoreUObject::FObjectHandle> Range{};

                        if (Anchor <= Clicked) {
                            Range.assign(Anchor, Clicked + 1);
                        } else {
                            Range.assign(std::make_reverse_iterator(Anchor + 1), std::make_reverse_iterator(Clicked));
                        }

                        Context->SetSelectedActors(Range);
                    } else {
                        Context->SelectActor(Handle);
                    }
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    Context->OpenInSequencer(Handle);
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !Selected) {
                    Context->SelectActor(Handle);
                }

                if (ImGui::BeginPopupContextItem("ActorMenu")) {
                    DuplicateRequested = ImGui::MenuItem("Duplicate", nullptr, false, CanEdit) || DuplicateRequested;
                    DeleteRequested = ImGui::MenuItem("Delete", "Del", false, CanEdit) || DeleteRequested;

                    ImGui::Separator();

                    if (ImGui::MenuItem("Use as PIE Camera", nullptr, false, CanEdit && Context->GetSelectedCamera() != nullptr)) {
                        Context->UseSelectedCamera();
                    }

                    ImGui::EndPopup();
                }

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, RowAlpha);
                ObjectRow::DrawType(Actor);
                ImGui::PopStyleVar();
                ImGui::PopID();
            }

            if (Visible.empty() && Filter.IsActive()) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextDisabled("검색 결과가 없습니다.");
            }

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
                Context->ClearSelection();
            }

            ImGui::EndTable();
        }

        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
            DeleteRequested = CanEdit && Context->GetSelectedActor() != nullptr;
        }

        // 순회 중인 핸들 목록이 바뀌지 않도록 테이블을 닫은 뒤 적용합니다.
        if (DuplicateRequested) {
            Context->DuplicateSelectedActors();
        }

        if (DeleteRequested) {
            Context->DestroySelectedActor();
        }

        const Engine::UCameraComponent* Camera{Context->GetWorld() != nullptr ? Context->GetWorld()->GetActiveCamera() : nullptr};

        ImGui::TextDisabled("%zu actors (%zu selected) | PIE Camera: %s", Actors.size(), Selection.size(), Camera != nullptr ? Camera->GetOwner()->GetName().c_str() : "None");
    }

}
