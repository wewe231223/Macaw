#include "pch.h"
#include "Editor/Panel/FMaterialEditorPanel.h"
#include "Asset/FAssetRegistry.h"
#include "Asset/USurfaceOpaque.h"
#include "Asset/UTexture.h"
#include "Editor/View/FAssetThumbnailRenderer.h"
#include "Render/Renderer.h"
#include "Editor/Property/FReflectedDetails.h"
#include "CoreUObject/Reflection/UStruct.h"

#include <algorithm>
#include <array>

namespace {
    constexpr Uint32 PreviewSize{512};


}

FMaterialEditorPanel::FMaterialEditorPanel(FAssetThumbnailRenderer& InThumbnailRenderer)
	: FEditorWindow{"Material Editor###MaterialEditor", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking},
	  mThumbnailRenderer{InThumbnailRenderer} {
    SetVisible(false);
    mPropertyEditor.BindThumbnailRenderer(&InThumbnailRenderer);
}

FMaterialEditorPanel::~FMaterialEditorPanel() = default;

void FMaterialEditorPanel::OpenMaterial(FAssetHandle MaterialHandle) {
    if (FAssetRegistry::Get().ResolveAsset<USurfaceOpaque>(MaterialHandle) == nullptr) {
        return;
    }

    mMaterialHandle = MaterialHandle;
    mPreviewDirty = true;
    SetVisible(true);
}

void FMaterialEditorPanel::RenderOffscreen(FRenderer& Renderer) {
    if (!mPreviewDirty || FAssetRegistry::Get().ResolveAsset<USurfaceOpaque>(mMaterialHandle) == nullptr) {
        return;
    }

    if (!mPreviewSurface.IsValid()) {
        mPreviewSurface.Initialize(Renderer.GetDevice(), PreviewSize, PreviewSize);
    }

    if (mPreviewSurface.IsValid()) {
        mThumbnailRenderer.RenderThumbnail(mMaterialHandle);
        mThumbnailRenderer.RenderMaterialPreview(mMaterialHandle, mPreviewSurface);
        mPreviewDirty = false;
    }
}

void FMaterialEditorPanel::ReleaseRenderResources() {
    mPreviewSurface.Reset();
    mPreviewDirty = true;
}

void FMaterialEditorPanel::DrawContents() {
    USurfaceOpaque* Material{FAssetRegistry::Get().ResolveAsset<USurfaceOpaque>(mMaterialHandle)};

    if (Material == nullptr) {
        ImGui::TextDisabled("Material unavailable.");
        return;
    }

    const FAssetPath* AssetPath{FAssetRegistry::Get().GetAssetPath(mMaterialHandle)};

    ImGui::TextUnformatted(AssetPath != nullptr ? AssetPath->mPath.c_str() : Material->GetAssetName().c_str());

    const float PreviewWidth{std::min(ImGui::GetContentRegionAvail().x, 400.0f)};

    if (ID3D11ShaderResourceView* Preview{mPreviewSurface.GetShaderResourceView()}; Preview != nullptr) {
        ImGui::Image(ImTextureRef{reinterpret_cast<ImTextureID>(Preview)}, ImVec2{PreviewWidth, PreviewWidth});
    } else {
        ImGui::Dummy(ImVec2{PreviewWidth, PreviewWidth});
    }

    ImGui::Separator();

    FReflectedDetails::Draw(*Material->GetClass(), Material, mPropertyEditor, [this, Material](const FProperty& Property, const FPropertyValue& Value) {
        if (Property.SetValue(*Material, Value)) {
            mPreviewDirty = true;
        }
    });

    if (ImGui::Button("Save Material")) {
        FAssetRegistry::Get().SaveMaterial(mMaterialHandle);
    }
}
