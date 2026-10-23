#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "Editor/Panel/FPropertyEditorContext.h"

#include "ImGui/imgui.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UAsset.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Asset/UTexture.h"
#include "Asset/UMesh.h"
#include "Asset/UMaterial.h"
#include "Editor/View/FAssetThumbnailRenderer.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <vector>

namespace {
    constexpr char StaticMeshAssetPayloadType[]{"MACAW_STATIC_MESH_ASSET"};
    constexpr char MaterialAssetPayloadType[]{"MACAW_MATERIAL_ASSET"};
    constexpr char TextureAssetPayloadType[]{"MACAW_TEXTURE_ASSET"};

    void ApplyAssetBrowserDrop(const UClass& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) {
        const IAssetRegistry* Registry{FAssetRegistry::TryGet()};
        const char* PayloadType{};

        if (AssetType.IsChildOf(UMesh::StaticClass())) {
            PayloadType = StaticMeshAssetPayloadType;
        } else if (AssetType.IsChildOf(UMaterial::StaticClass())) {
            PayloadType = MaterialAssetPayloadType;
        } else if (AssetType.IsChildOf(UTexture::StaticClass())) {
            PayloadType = TextureAssetPayloadType;
        }

        if (Registry == nullptr || PayloadType == nullptr || !ImGui::BeginDragDropTarget()) {
            return;
        }

        const ImGuiPayload* Payload{ImGui::AcceptDragDropPayload(PayloadType)};

        if (Payload != nullptr && Payload->DataSize == sizeof(FAssetHandle)) {
            FAssetHandle DroppedHandle{};

            std::memcpy(&DroppedHandle, Payload->Data, sizeof(DroppedHandle));

            const UAsset* DroppedAsset{Registry->ResolveAsset<UAsset>(DroppedHandle)};

            if (DroppedAsset != nullptr && DroppedAsset->GetClass()->IsChildOf(&AssetType) && DroppedHandle != CurrentHandle) {
                Setter(DroppedHandle);
            }
        }

        ImGui::EndDragDropTarget();
    }
}

bool FPropertyEditorContext::BeginCategory(const char* Label, bool BDefaultOpen) const {
    return ImGui::CollapsingHeader(Label, BDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

void FPropertyEditorContext::DrawDisabledText(const char* Text) const {
    ImGui::TextDisabled("%s", Text);
}

void FPropertyEditorContext::DrawButton(const char* Label, const std::function<void()>& OnClicked) const {
    if (ImGui::Button(Label)) {
        OnClicked();
    }
}

void FPropertyEditorContext::DrawBool(const char* Label, bool Value, const std::function<void(bool)>& Setter) const {
    if (ImGui::Checkbox(Label, &Value)) {
        Setter(Value);
    }
}

void FPropertyEditorContext::DrawInteger(const char* Label, Int64 Value, float Speed, Int64 Min, Int64 Max, const std::function<void(Int64)>& Setter) const {
    if (ImGui::DragScalar(Label, ImGuiDataType_S64, &Value, Speed, &Min, &Max, nullptr, ImGuiSliderFlags_AlwaysClamp)) {
        Setter(std::clamp(Value, Min, Max));
    }
}

void FPropertyEditorContext::DrawFloat(const char* Label, float Value, float Speed, float Min, float Max, const std::function<void(float)>& Setter) const {
    if (ImGui::DragFloat(Label, &Value, Speed, Min, Max, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        Setter(std::clamp(Value, Min, Max));
    }
}

void FPropertyEditorContext::DrawVector2(const char* Label, const FVector2& Value, float Speed, float Min, float Max, const std::function<void(const FVector2&)>& Setter) const {
    FVector2 EditedValue{Value};

    if (ImGui::DragFloat2(Label, &EditedValue.mX, Speed, Min, Max, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        Setter(FVector2{std::clamp(EditedValue.mX, Min, Max), std::clamp(EditedValue.mY, Min, Max)});
    }
}

void FPropertyEditorContext::DrawVector3(const char* Label, const FVector3& Value, float Speed, float Min, float Max, const std::function<void(const FVector3&)>& Setter) const {
    FVector3 EditedValue{Value};

    if (ImGui::DragFloat3(Label, &EditedValue.mX, Speed, Min, Max, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
        Setter(FVector3{std::clamp(EditedValue.mX, Min, Max), std::clamp(EditedValue.mY, Min, Max), std::clamp(EditedValue.mZ, Min, Max)});
    }
}

void FPropertyEditorContext::DrawColor(const char* Label, const FVector4& Value, const std::function<void(const FVector4&)>& Setter) const {
    FVector4 EditedValue{Value};

    if (ImGui::ColorEdit4(Label, &EditedValue.mX)) {
        Setter(EditedValue);
    }
}

void FPropertyEditorContext::DrawText(const char* Label, const FString& Value, const std::function<void(const FString&)>& Setter) const {
    std::array<char, 2048> TextBuffer{};
    const std::size_t CopyLength{std::min(Value.size(), TextBuffer.size() - 1)};

    std::memcpy(TextBuffer.data(), Value.data(), CopyLength);

    if (ImGui::InputTextMultiline(Label, TextBuffer.data(), TextBuffer.size(), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 5.0f))) {
        Setter(TextBuffer.data());
    }
}

void FPropertyEditorContext::DrawTransform(const char* Label, const FTransform& Value, const std::function<void(const FTransform&)>& Setter) {
    FRotationEditorState& RotationState{mRotationEdits[ImGui::GetID(Label)]};
    FVector3 Position{Value.GetPosition()};
    FRotator Rotation{RotationState.Update(Value.GetRotationQuaternion())};
    FVector3 Scale{Value.GetScale()};
    FTransform Transform{Value};
    bool Changed{false};

    ImGui::PushID(Label);

    if (ImGui::DragFloat3("Position", &Position.X, 0.1f)) {
        Transform.SetPosition(Position);
        Changed = true;
    }

    if (ImGui::DragFloat3("Rotation", &Rotation.Pitch, 0.5f)) {
        RotationState.SetRotation(Rotation);
        Transform.SetRotation(Rotation);
        Changed = true;
    }

    if (ImGui::DragFloat3("Scale", &Scale.X, 0.05f, 0.001f, FLT_MAX)) {
        Transform.SetScale(FVector::Max(Scale, FVector{0.001f, 0.001f, 0.001f}));
        Changed = true;
    }

    ImGui::PopID();

    if (Changed) {
        Setter(Transform);
    }
}

void FPropertyEditorContext::DrawReferencePicker(const char* Label, const char* Preview, bool BNoneSelected, const std::function<void()>& ClearSelection, const std::vector<FPropertyReferenceOption>& Options) const {
    if (!ImGui::BeginCombo(Label, Preview)) {
        return;
    }

    if (ImGui::Selectable("None", BNoneSelected)) {
        ClearSelection();
    }

    for (const FPropertyReferenceOption& Option : Options) {
        ImGui::PushID(Option.mLabel.c_str());
        ImGui::PushID(Option.mId);

        if (ImGui::Selectable(Option.mLabel.c_str(), Option.mBSelected)) {
            Option.mOnSelected();
        }

        ImGui::PopID();
        ImGui::PopID();
    }

    ImGui::EndCombo();
}

void FPropertyEditorContext::DrawAssetPicker(const char* Label, const UClass& AssetType, FAssetHandle CurrentHandle, const std::function<void(FAssetHandle)>& Setter) const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr) {
        ImGui::TextDisabled("%s: Asset registry unavailable", Label);
        return;
    }

    const UAsset* Current{Registry->ResolveAsset<UAsset>(CurrentHandle)};

    if (Current != nullptr && !Current->GetClass()->IsChildOf(&AssetType)) {
        Current = nullptr;
    }

    const FString PreviewName{Current != nullptr ? Current->GetAssetName() : FString{"None"}};
    const bool BSupportsThumbnail{SupportsAssetThumbnail(AssetType)};

    ImGui::PushID(Label);

    if (BSupportsThumbnail) {
        ID3D11ShaderResourceView* Thumbnail{mThumbnailRenderer != nullptr ? mThumbnailRenderer->GetThumbnail(CurrentHandle) : nullptr};

        if (Thumbnail != nullptr) {
            ImGui::Image(ImTextureRef(reinterpret_cast<ImTextureID>(Thumbnail)), ImVec2(48.0f, 48.0f));
        } else {
            ImGui::Button("##AssetThumbnail", ImVec2(48.0f, 48.0f));
        }

        ApplyAssetBrowserDrop(AssetType, CurrentHandle, Setter);
        ImGui::SameLine();
    }

    if (ImGui::BeginCombo(Label, PreviewName.c_str())) {
        ImGui::PushID("None");

        if (DrawAssetOption("None", nullptr, Current == nullptr)) {
            Setter({});
        }

        ImGui::PopID();

        TArray<FAssetHandle> MatchingAssets{Registry->GetAssetHandles(AssetType)};
        std::ranges::sort(MatchingAssets, [Registry](FAssetHandle Left, FAssetHandle Right) {
            return Registry->GetAssetPath(Left)->mPath < Registry->GetAssetPath(Right)->mPath;
        });

        for (FAssetHandle Handle : MatchingAssets) {
            const FString& AssetPath{Registry->GetAssetPath(Handle)->mPath};
            const std::size_t NameOffset{AssetPath.find_last_of('/') + 1};
            const char* AssetName{AssetPath.c_str() + NameOffset};
            ID3D11ShaderResourceView* Thumbnail{mThumbnailRenderer != nullptr ? mThumbnailRenderer->GetThumbnail(Handle) : nullptr};

            ImGui::PushID(Registry->ResolveAsset<UObject>(Handle));

            if (DrawAssetOption(AssetName, Thumbnail, Handle == CurrentHandle) && Handle != CurrentHandle) {
                Setter(Handle);
            }

            if (Handle == CurrentHandle) {
                ImGui::SetItemDefaultFocus();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", AssetPath.c_str());
            }

            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImGui::PopID();
}

bool FPropertyEditorContext::SupportsAssetThumbnail(const UClass& AssetType) const {
    return UMesh::StaticClass()->IsChildOf(&AssetType) || UMaterial::StaticClass()->IsChildOf(&AssetType) || UTexture::StaticClass()->IsChildOf(&AssetType);
}

bool FPropertyEditorContext::DrawAssetOption(const char* Label, ID3D11ShaderResourceView* Thumbnail, bool BSelected) const {
    constexpr float RowHeight{36.0f};
    constexpr float ImageSize{32.0f};
    constexpr float Padding{2.0f};

    const ImVec2 RowMin{ImGui::GetCursorScreenPos()};
    const ImVec2 RowSize{ImGui::GetContentRegionAvail().x, RowHeight};
    const bool BPressed{ImGui::Selectable("##AssetOption", BSelected, ImGuiSelectableFlags_None, RowSize)};

    ImDrawList* DrawList{ImGui::GetWindowDrawList()};
    const ImVec2 ImageMin{RowMin.x + Padding, RowMin.y + Padding};
    const ImVec2 ImageMax{ImageMin.x + ImageSize, ImageMin.y + ImageSize};

    if (Thumbnail != nullptr) {
        DrawList->AddImage(reinterpret_cast<ImTextureID>(Thumbnail), ImageMin, ImageMax);
    } else {
        DrawList->AddRect(ImageMin, ImageMax, ImGui::GetColorU32(ImGuiCol_Border));
    }

    const float TextY{RowMin.y + (RowHeight - ImGui::GetTextLineHeight()) * 0.5f};

    DrawList->AddText(ImVec2(ImageMax.x + 6.0f, TextY), ImGui::GetColorU32(ImGuiCol_Text), Label);

    return BPressed;
}

void FPropertyEditorContext::BindThumbnailRenderer(FAssetThumbnailRenderer* InThumbnailRenderer) {
    mThumbnailRenderer = InThumbnailRenderer;
}

void FPropertyEditorContext::BeginProperty(const char* Label) const {
    ImGui::PushID(Label);
}

void FPropertyEditorContext::EndProperty() const {
    ImGui::PopID();
}
