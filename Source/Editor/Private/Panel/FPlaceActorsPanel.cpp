/**
 * @file FPlaceActorsPanel.cpp
 * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FPlaceActorsPanel.h"
#include "Editor/FEditorContext.h"
#include "Editor/FAssetDragDrop.h"
#include "Engine/FProject.h"
#include "Engine/FAssetManager.h"
#include "Engine/UTexture2D.h"
#include "Engine/AActor.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>

namespace Editor {

    Editor::FPlaceActorsPanel::FPlaceActorsPanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Place Actors", true} {
    }

    Editor::FPlaceActorsPanel::~FPlaceActorsPanel() = default;

    void Editor::FPlaceActorsPanel::DrawContent(float) {
        const char* Categories[]{"All", "Lights", "Basic", "Visual Effects"};

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##Category", &mCategory, Categories, 4);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##SearchActors", "Search actors", mSearch.data(), mSearch.size());
        ImGui::TextWrapped("Drag an actor into a viewport, or double-click to place it.");
        ImGui::Separator();
        ImGui::BeginDisabled(!GetEditorContext()->CanEditWorld());

        if (mCategory == 0 || mCategory == 2) {
            DrawEntry("Cube", "Cube", "StaticMeshComponent", nullptr);
            DrawEntry("Sphere", "Sphere", "StaticMeshComponent", nullptr);
            DrawEntry("Plane", "Plane", "StaticMeshComponent", nullptr);
            DrawEntry("Text", "Text", "BillboardTextComponent", nullptr);
            DrawEntry("Billboard", "Billboard", "BillboardComponent", "Pawn_64x.png");
        }

        for (const CoreUObject::UClass* Class : CoreUObject::FReflectionRegistry::GetDerivedClasses(Engine::AActor::StaticClass())) {
            if (!Class->CanCreate()) {
                continue;
            }

            const Core::FString& Name{Class->GetName()};
            const bool Light{Name.find("Light") != Core::FString::npos};
            const bool Fog{Name.find("Fog") != Core::FString::npos};

            if ((mCategory == 1 && !Light) || (mCategory == 2 && (Light || Fog)) || (mCategory == 3 && !Fog)) {
                continue;
            }

            const char* Icon{Fog ? "ExponentialHeightFog_64x.png" : Name.find("Directional") != Core::FString::npos ? "DirectionalLight_64x.png" : Name.find("Ambient") != Core::FString::npos ? "SkyLight_64x.png" : Name.find("Spot") != Core::FString::npos ? "SpotLight_64x.png" : Light ? "PointLight_64x.png" : nullptr};
            Core::FString Label{Name.starts_with('A') ? Name.substr(1) : Name};
            DrawEntry(Name, Label, Fog ? "HeightFogComponent" : Light ? "LightComponent" : Name.find("Camera") != Core::FString::npos ? "CameraComponent" : "Default components", Icon);
        }

        ImGui::EndDisabled();
    }

    void Editor::FPlaceActorsPanel::DrawEntry(const Core::FString& Preset, const Core::FString& Label, const char* Components, const char* Icon) {
        const ImGuiTextFilter Filter{mSearch.data()};

        if (!Filter.PassFilter(Label.c_str())) {
            return;
        }

        ImGui::PushID(Preset.c_str());

        if (Icon != nullptr && GetEditorContext()->GetProject() != nullptr) {
            Core::FString Error{};
            Engine::UTexture2D* Texture{GetEditorContext()->GetProject()->GetAssets()->LoadTexture(Core::FString{"Editor/Icon/"} + Icon, Error)};

            if (Texture != nullptr && Texture->CreateResources(static_cast<ID3D11Device*>(GetEditorContext()->GetRenderer()->GetNativeDevice()))) {
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Texture->GetShaderResourceView()))}, {32.0f, 32.0f});
                ImGui::SameLine();
            }
        }

        ImGui::Selectable(Label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, {0.0f, 32.0f});

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            GetEditorContext()->SpawnActorPreset(Preset);
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(Editor::EditorAssetDragDrop::ActorPayload, Preset.c_str(), Preset.size() + 1);
            ImGui::TextUnformatted(Label.c_str());
            ImGui::TextDisabled("%s", Components);
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s\n%s", Label.c_str(), Components);
        }

        ImGui::PopID();
    }

}
