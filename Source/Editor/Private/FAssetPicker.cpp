/**
 * @file FAssetPicker.cpp
 * @brief 공유 에셋 선택기의 썸네일·검색·드롭 구현입니다.
 * @ingroup Editor
 */
#include "Editor/FAssetPicker.h"
#include "Editor/FEditorAssetCatalog.h"
#include "Editor/FAssetThumbnailCache.h"
#include "Editor/FAssetDragDrop.h"
#include "Core/FilePath.h"

#include <imgui.h>
#include <algorithm>

namespace Editor {

    bool Editor::FAssetPicker::AcceptDrop(Editor::FEditorAssetCatalog& Catalog, Core::FString& Path, Asset::EAssetType Type) {
        bool Changed{};

        if (ImGui::BeginDragDropTarget()) {
            const char* PayloadType{Type == Asset::EAssetType::Mesh ? Editor::EditorAssetDragDrop::MeshPayload : Type == Asset::EAssetType::Material ? Editor::EditorAssetDragDrop::MaterialPayload : Editor::EditorAssetDragDrop::TexturePayload};

            if (const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(PayloadType)}; Payload != nullptr && Payload->Data != nullptr && Payload->DataSize > 1 && static_cast<const char*>(Payload->Data)[Payload->DataSize - 1] == '\0') {
                const Core::FString Candidate{static_cast<const char*>(Payload->Data), static_cast<std::size_t>(Payload->DataSize - 1)};
                const auto* Entry{Catalog.Find(Core::FilePath::FromUtf8(Candidate))};

                if (Entry != nullptr && Entry->GetType() == Type) {
                    Path = Core::FilePath::ToGenericUtf8(Entry->GetPath());
                    Changed = true;
                }
            }

            ImGui::EndDragDropTarget();
        }

        return Changed;
    }

    bool Editor::FAssetPicker::DrawOptions(Editor::FEditorAssetCatalog& Catalog, Editor::FAssetThumbnailCache& Thumbnails, Core::FString& Path, Asset::EAssetType Type, const char* EmptyLabel) {
        if (ImGui::IsWindowAppearing()) {
            mSearch.fill('\0');
            ImGui::SetKeyboardFocusHere();
        }

        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputTextWithHint("##Search", "Search assets...", mSearch.data(), mSearch.size());

        const ImGuiTextFilter Filter{mSearch.data()};
        bool Changed{};

        if (ImGui::Selectable(EmptyLabel, Path.empty())) {
            Path.clear();
            Changed = true;
        }

        ImGui::Separator();

        const float RowHeight{56.0f};
        const float ContentOffsetY{10.0f};

        if (ImGui::BeginChild("##AssetOptions", {0.0f, 280.0f})) {
            for (const auto& Entry : Catalog.GetEntries()) {
                const Core::FString Actual{Core::FilePath::ToGenericUtf8(Entry.GetPath())};

                if (Entry.GetType() != Type || !Filter.PassFilter(Actual.c_str())) {
                    continue;
                }

                ImGui::PushID(Actual.c_str());

                const ImVec2 Origin{ImGui::GetCursorScreenPos()};
                const bool Visible{ImGui::IsRectVisible({ImGui::GetContentRegionAvail().x, RowHeight})};

                if (ImGui::Selectable("##Asset", Path == Actual, ImGuiSelectableFlags_None, {0.0f, RowHeight})) {
                    Path = Actual;
                    Changed = true;
                }

                const ImVec2 Next{ImGui::GetCursorScreenPos()};

                if (Visible) {
                    ImGui::SetCursorScreenPos({Origin.x + 3.0f, Origin.y + ContentOffsetY});

                    void* Thumbnail{Thumbnails.GetThumbnail(Entry.GetPath(), Type)};

                    if (Thumbnail != nullptr) {
                        ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {36.0f, 36.0f});
                    } else {
                        ImGui::Dummy({36.0f, 36.0f});
                    }

                    ImGui::SetCursorScreenPos({Origin.x + 46.0f, Origin.y + ContentOffsetY});
                    ImGui::TextUnformatted(Core::FilePath::ToUtf8(Entry.GetPath().stem()).c_str());
                    ImGui::SetCursorScreenPos({Origin.x + 46.0f, Origin.y + ContentOffsetY + 20.0f});
                    ImGui::TextDisabled("%s", Core::FilePath::ToGenericUtf8(Entry.GetPath().parent_path()).c_str());
                    ImGui::SetCursorScreenPos(Next);
                    ImGui::Dummy({0.0f, 0.0f});
                }

                ImGui::PopID();
            }
        }

        ImGui::EndChild();

        return Changed;
    }

    bool Editor::FAssetPicker::Draw(Editor::FEditorAssetCatalog& Catalog, Editor::FAssetThumbnailCache& Thumbnails, const char* Label, Core::FString& Path, Asset::EAssetType Type, const Core::FString& InheritedPath, const char* EmptyLabel, const char* InheritedLabel) {
        ImGui::PushID(Label);

        const Core::FString Effective{Path.empty() ? InheritedPath : Path};
        const Core::FString Display{Effective.empty() ? Core::FString{EmptyLabel} : Effective};
        Core::FString Name{Effective.empty() ? Core::FString{EmptyLabel} : Core::FilePath::ToUtf8(Core::FilePath::FromUtf8(Display).stem())};

        if (Path.empty() && !InheritedPath.empty()) {
            Name += " (" + Core::FString{InheritedLabel} + ")";
        }

        ImGui::TextUnformatted(Label);

        const bool Visible{ImGui::IsRectVisible({52.0f, 52.0f})};
        void* Thumbnail{Visible && (!Effective.empty() || Type != Asset::EAssetType::Texture) ? Thumbnails.GetThumbnail(Core::FilePath::FromUtf8(Effective), Type) : nullptr};

        if (Thumbnail != nullptr) {
            if (ImGui::ImageButton("##Thumbnail", ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {48.0f, 48.0f})) {
                ImGui::OpenPopup("##ThumbnailPicker");
            }
        } else if (ImGui::Button("...##Thumbnail", {54.0f, 54.0f})) {
            ImGui::OpenPopup("##ThumbnailPicker");
        }

        bool Changed{AcceptDrop(Catalog, Path, Type)};

        if (ImGui::IsItemHovered() && !Effective.empty()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(Display.c_str());

            if (Thumbnail != nullptr) {
                ImGui::Image(ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(Thumbnail))}, {144.0f, 144.0f});
            }

            ImGui::EndTooltip();
        }

        ImGui::SameLine();

        const float ComboWidth{std::max(60.0f, ImGui::GetContentRegionAvail().x)};
        ImGui::SetNextItemWidth(ComboWidth);
        ImGui::SetNextWindowSizeConstraints({280.0f, 0.0f}, {std::max(480.0f, ComboWidth), 400.0f});

        if (ImGui::BeginCombo("##Asset", Name.c_str())) {
            if (DrawOptions(Catalog, Thumbnails, Path, Type, EmptyLabel)) {
                Changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndCombo();
        }

        Changed = AcceptDrop(Catalog, Path, Type) || Changed;
        ImGui::SetNextWindowSizeConstraints({280.0f, 0.0f}, {480.0f, 400.0f});

        if (ImGui::BeginPopup("##ThumbnailPicker")) {
            if (DrawOptions(Catalog, Thumbnails, Path, Type, EmptyLabel)) {
                Changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::PopID();

        return Changed;
    }

}
