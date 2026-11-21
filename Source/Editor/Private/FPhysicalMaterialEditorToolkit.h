#pragma once

#include "FAssetEditorToolkit.h"
#include "Editor/FEditorContext.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"

namespace Editor {
    class FPhysicalMaterialEditorToolkit final : public Editor::FAssetEditorToolkit {
    public:
        FPhysicalMaterialEditorToolkit(Editor::FEditorContext& Context, const std::filesystem::path& Path, const PhysicsRuntime::UPhysicalMaterial& Material, Editor::FMessageBox& Messages);
        ~FPhysicalMaterialEditorToolkit() override;

    public:
        std::optional<Editor::FMessageBox::FSavePrompt> GetClosePrompt() override;

    private:
        void UpdateEditor(float DeltaTime) override;
        void DrawDetails();

        bool Save();
        bool Discard();
        bool RequestTabClose();

    private:
        Editor::FEditorContext& mContext;
        Editor::FMessageBox& mMessages;
        PhysicsRuntime::UPhysicalMaterial mDraft{};
        Core::FString mError{};
        bool mDirty{};
    };
}
