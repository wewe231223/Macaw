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

FPlaceActorsPanel::FPlaceActorsPanel(FEditorContext* Context)
	: FEditorPanel{Context, "Place Actors", true} {
}

FPlaceActorsPanel::~FPlaceActorsPanel() = default;

void FPlaceActorsPanel::DrawContent(float) {
    const char* Categories[]{"All", "Lights", "Basic"};
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##Category", &mCategory, Categories, 3);
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##SearchActors", "Search actors", mSearch.data(), mSearch.size());
    ImGui::TextWrapped("Drag an actor into a viewport, or double-click to place it.");
    ImGui::Separator();
    ImGui::BeginDisabled(!GetEditorContext()->CanEditWorld());
    if (mCategory != 1) {
        DrawEntry("Cube", "Cube", "StaticMeshComponent", nullptr);
        DrawEntry("Sphere", "Sphere", "StaticMeshComponent", nullptr);
        DrawEntry("Plane", "Plane", "StaticMeshComponent", nullptr);
        DrawEntry("Text", "Text", "BillboardTextComponent", nullptr);
        DrawEntry("Billboard", "Billboard", "BillboardComponent", "Pawn_64x.png");
    }
    for (const UClass* Class : FReflectionRegistry::GetDerivedClasses(AActor::StaticClass())) {
        if (!Class->CanCreate()) {
            continue;
        }
        const FString& Name{Class->GetName()};
        const bool Light{Name.find("Light") != FString::npos};
        if ((mCategory == 1 && !Light) || (mCategory == 2 && Light)) {
            continue;
        }
        const char* Icon{Name.find("Directional") != FString::npos ? "DirectionalLight_64x.png" : Name.find("Ambient") != FString::npos ? "SkyLight_64x.png" : Name.find("Spot") != FString::npos ? "SpotLight_64x.png" : Light ? "PointLight_64x.png" : nullptr};
        FString Label{Name.starts_with('A') ? Name.substr(1) : Name};
        DrawEntry(Name, Label, Light ? "LightComponent" : Name.find("Camera") != FString::npos ? "CameraComponent" : "Default components", Icon);
    }
    ImGui::EndDisabled();
}

void FPlaceActorsPanel::DrawEntry(const FString& Preset, const FString& Label, const char* Components, const char* Icon) {
    const ImGuiTextFilter Filter{mSearch.data()};
    if (!Filter.PassFilter(Label.c_str())) {
        return;
    }
    ImGui::PushID(Preset.c_str());
    if (Icon != nullptr && GetEditorContext()->GetProject() != nullptr) {
        FString Error{};
        UTexture2D* Texture{GetEditorContext()->GetProject()->GetAssets()->LoadTexture(FString{"Editor/Icon/"} + Icon, Error)};
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
        ImGui::SetDragDropPayload(EditorAssetDragDrop::ActorPayload, Preset.c_str(), Preset.size() + 1);
        ImGui::TextUnformatted(Label.c_str());
        ImGui::TextDisabled("%s", Components);
        ImGui::EndDragDropSource();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s\n%s", Label.c_str(), Components);
    }
    ImGui::PopID();
}
