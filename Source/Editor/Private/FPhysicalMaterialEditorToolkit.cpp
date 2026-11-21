#include "FPhysicalMaterialEditorToolkit.h"
#include "Core/FilePath.h"
#include "Editor/FEditorToolkitHost.h"
#include "Engine/FProject.h"

#include <imgui.h>
#include <limits>
#include <utility>

namespace Editor {
    FPhysicalMaterialEditorToolkit::FPhysicalMaterialEditorToolkit(Editor::FEditorContext& Context, const std::filesystem::path& Path, const PhysicsRuntime::UPhysicalMaterial& Material, Editor::FMessageBox& Messages)
		: FAssetEditorToolkit{Path, Asset::EAssetType::PhysicalMaterial, "###PhysicalMaterialEditor:" + Core::FilePath::ToGenericUtf8(Path)},
		  mContext{Context},
		  mMessages{Messages} {
        mDraft.CopySettingsFrom(Material);

        GetToolkitHost().AddWidget("Details", "Details", [this](float) {
            DrawDetails();
        });

        GetTab().SetCanCloseHandler([this]() {
            return RequestTabClose();
        });
    }

    FPhysicalMaterialEditorToolkit::~FPhysicalMaterialEditorToolkit() = default;

    std::optional<Editor::FMessageBox::FSavePrompt> FPhysicalMaterialEditorToolkit::GetClosePrompt() {
        if (!mDirty) {
            return {};
        }

        return Editor::FMessageBox::FSavePrompt{"Physical Material", "Save changes to " + Core::FilePath::ToUtf8(GetAssetPath().filename()) + "?", [this]() {
            return Save();
        }, [this]() {
            return Discard();
        }};
    }

    void FPhysicalMaterialEditorToolkit::UpdateEditor(float) {
        GetTab().SetName(Core::FilePath::ToUtf8(GetAssetPath().stem()) + (mDirty ? "*" : "") + "###PhysicalMaterialEditor:" + Core::FilePath::ToGenericUtf8(GetAssetPath()));
    }

    void FPhysicalMaterialEditorToolkit::DrawDetails() {
        ImGui::BeginDisabled(!mContext.CanEditWorld());

        constexpr float MaximumFriction{std::numeric_limits<float>::max()};
        float StaticFriction{mDraft.GetStaticFriction()};
        float DynamicFriction{mDraft.GetDynamicFriction()};
        float Restitution{mDraft.GetRestitution()};

        if (ImGui::DragFloat("Static Friction", &StaticFriction, 0.01f, 0.0f, MaximumFriction, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            mDirty |= mDraft.SetStaticFriction(StaticFriction);
        }

        if (ImGui::DragFloat("Dynamic Friction", &DynamicFriction, 0.01f, 0.0f, MaximumFriction, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            mDirty |= mDraft.SetDynamicFriction(DynamicFriction);
        }

        if (ImGui::DragFloat("Restitution", &Restitution, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
            mDirty |= mDraft.SetRestitution(Restitution);
        }

        constexpr const char* Modes{"Average\0Min\0Multiply\0Max\0"};
        int FrictionMode{static_cast<int>(mDraft.GetFrictionCombineMode())};
        int RestitutionMode{static_cast<int>(mDraft.GetRestitutionCombineMode())};

        if (ImGui::Combo("Friction Combine Mode", &FrictionMode, Modes)) {
            mDirty |= mDraft.SetFrictionCombineMode(static_cast<PhysicsRuntime::EPhysicalMaterialCombineMode>(FrictionMode));
        }

        if (ImGui::Combo("Restitution Combine Mode", &RestitutionMode, Modes)) {
            mDirty |= mDraft.SetRestitutionCombineMode(static_cast<PhysicsRuntime::EPhysicalMaterialCombineMode>(RestitutionMode));
        }

        ImGui::BeginDisabled(!mDirty);

        if (ImGui::Button("Save")) {
            Save();
        }

        ImGui::EndDisabled();
        ImGui::EndDisabled();

        if (!mError.empty()) {
            ImGui::TextWrapped("%s", mError.c_str());
        }
    }

    bool FPhysicalMaterialEditorToolkit::Save() {
        if (!mContext.GetProject()->GetAssets()->SavePhysicalMaterial(GetAssetPath(), mDraft, mError)) {
            return false;
        }

        mDirty = false;
        NotifyChanged();

        return true;
    }

    bool FPhysicalMaterialEditorToolkit::Discard() {
        const auto* Material{mContext.GetProject()->GetAssets()->LoadPhysicalMaterial(GetAssetPath(), mError)};

        if (Material == nullptr) {
            return false;
        }

        mDraft.CopySettingsFrom(*Material);
        mDirty = false;

        return true;
    }

    bool FPhysicalMaterialEditorToolkit::RequestTabClose() {
        auto Prompt{GetClosePrompt()};

        if (!Prompt) {
            return true;
        }

        mMessages.ConfirmSave({std::move(*Prompt)}, [this]() {
            GetTab().SetVisible(false);
        });

        return false;
    }
}
