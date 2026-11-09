/**
 * @file FPlaceActorsPanel.cpp
 * @brief 검색과 분류를 통해 액터 프리셋을 배치하는 에디터 패널입니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FPlaceActorsPanel.h"
#include "Editor/FEditorContext.h"
#include "Editor/FAssetDragDrop.h"
#include "Engine/FProject.h"
#include "Asset/FAssetManager.h"
#include "Asset/UTexture2D.h"
#include "Engine/AActor.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/UClass.h"
#include "RenderCore/IRendererModule.h"

#include <imgui.h>

namespace Editor {

    /**
     * @brief 편집 컨텍스트를 연결하고 액터 배치 패널을 생성합니다.
     */
    Editor::FPlaceActorsPanel::FPlaceActorsPanel(Editor::FEditorContext* Context)
		: Editor::FEditorPanel{Context, "Place Actors", true} {
    }

    /**
     * @brief FPlaceActorsPanel 객체의 수명을 끝내고 소유 자원을 정리합니다.
     */
    Editor::FPlaceActorsPanel::~FPlaceActorsPanel() = default;

    /**
     * @brief 액터 배치 아이콘을 다음 갱신 시 다시 생성하도록 표시합니다.
     */
    void Editor::FPlaceActorsPanel::InvalidateIcons() {
        mIconsDirty = true;
    }

    /**
     * @brief 액터 배치 목록의 캐시된 아이콘 텍스처를 해제합니다.
     */
    void Editor::FPlaceActorsPanel::ClearIcons() {
        mIcons = {};
        mIconsDirty = true;
    }

    /**
     * @brief 프로젝트 에셋과 렌더러에서 액터 배치 아이콘을 준비합니다.
     */
    void Editor::FPlaceActorsPanel::UpdateIcons() {
        if (!mIconsDirty) {
            return;
        }

        const Editor::FEditorContext* Context{GetEditorContext()};

        if (Context == nullptr || Context->GetProject() == nullptr || Context->GetRenderer() == nullptr) {
            return;
        }

        ID3D11Device* Device{static_cast<ID3D11Device*>(Context->GetRenderer()->GetNativeDevice())};

        if (Device == nullptr) {
            return;
        }

        static constexpr std::array<const char*, static_cast<std::size_t>(Editor::EPlaceActorIcon::Count)> Paths{nullptr, "Editor/Icon/Pawn_64x.png", "Editor/Icon/SkyLight_64x.png", "Editor/Icon/DirectionalLight_64x.png", "Editor/Icon/PointLight_64x.png", "Editor/Icon/SpotLight_64x.png", "Editor/Icon/ExponentialHeightFog_64x.png"};

        mIcons = {};

        for (std::size_t Index{1}; Index < Paths.size(); ++Index) {
            Core::FString Error{};
            Asset::UTexture2D* Texture{Context->GetProject()->GetAssets()->LoadTexture(Paths[Index], Error)};

            if (Texture != nullptr && Texture->CreateResources(Device)) {
                mIcons[Index] = Texture->GetShaderResourceView();
            }
        }

        mIconsDirty = false;
    }

    /**
     * @brief 검색·분류와 액터 배치 항목을 패널 안에 그립니다.
     */
    void Editor::FPlaceActorsPanel::DrawContent(float) {
        UpdateIcons();

        const char* Categories[]{"All", "Lights", "Basic", "Visual Effects"};

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::Combo("##Category", &mCategory, Categories, 4);
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##SearchActors", "Search actors", mSearch.data(), mSearch.size());
        ImGui::Separator();
        ImGui::BeginDisabled(!GetEditorContext()->CanEditWorld());

        if (mCategory == 0 || mCategory == 2) {
            DrawEntry("Cube", "Cube", "StaticMeshComponent", Editor::EPlaceActorIcon::None);
            DrawEntry("Sphere", "Sphere", "StaticMeshComponent", Editor::EPlaceActorIcon::None);
            DrawEntry("Plane", "Plane", "StaticMeshComponent", Editor::EPlaceActorIcon::None);
            DrawEntry("Text", "Text", "BillboardTextComponent", Editor::EPlaceActorIcon::None);
            DrawEntry("Billboard", "Billboard", "BillboardComponent", Editor::EPlaceActorIcon::Billboard);
        }

        for (const CoreUObject::UClass* Class : CoreUObject::FReflectionRegistry::GetDerivedClasses(Engine::AActor::StaticClass())) {
            if (!Class->CanCreate()) {
                continue;
            }

            const Core::FString& Name{Class->GetName()};
            const bool Light{Name.find("Light") != Core::FString::npos};
            const bool Fog{Name.find("Fog") != Core::FString::npos};
            const bool Decal{Name == "ADecalActor"};

            if ((mCategory == 1 && !Light) || (mCategory == 2 && (Light || Fog || Decal)) || (mCategory == 3 && !(Fog || Decal))) {
                continue;
            }

            const Editor::EPlaceActorIcon Icon{Fog ? Editor::EPlaceActorIcon::HeightFog : Name.find("Directional") != Core::FString::npos ? Editor::EPlaceActorIcon::DirectionalLight : Name.find("Ambient") != Core::FString::npos ? Editor::EPlaceActorIcon::AmbientLight : Name.find("Spot") != Core::FString::npos ? Editor::EPlaceActorIcon::SpotLight : Light ? Editor::EPlaceActorIcon::PointLight : Editor::EPlaceActorIcon::None};
            Core::FString Label{Name.starts_with('A') ? Name.substr(1) : Name};

            DrawEntry(Name, Label, Decal ? "DecalComponent" : Fog ? "HeightFogComponent" : Light ? "LightComponent" : Name.find("Camera") != Core::FString::npos ? "CameraComponent" : "Default components", Icon);
        }

        ImGui::EndDisabled();
    }

    /**
     * @brief 아이콘·설명과 드래그 페이로드를 갖는 액터 프리셋 항목을 그립니다.
     */
    void Editor::FPlaceActorsPanel::DrawEntry(const Core::FString& Preset, const Core::FString& Label, const char* Components, Editor::EPlaceActorIcon Icon) {
        const ImGuiTextFilter Filter{mSearch.data()};

        if (!Filter.PassFilter(Label.c_str())) {
            return;
        }

        ImGui::PushID(Preset.c_str());

        if (ID3D11ShaderResourceView* Resource{mIcons[static_cast<std::size_t>(Icon)].Get()}; Resource != nullptr) {
            ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Resource))}, {32.0f, 32.0f});
            ImGui::SameLine();
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
