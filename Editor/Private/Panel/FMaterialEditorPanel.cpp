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

FMaterialEditorPanel::FMaterialEditorPanel(FAssetRegistry& InRegistry, FAssetThumbnailRenderer& InThumbnailRenderer)
	: FEditorWindow("Material Editor###MaterialEditor", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking),
	  mRegistry(InRegistry),
	  mThumbnailRenderer(InThumbnailRenderer) {
    SetVisible(false);
    mPropertyEditor.BindAssetRegistry(&InRegistry);
    mPropertyEditor.BindThumbnailRenderer(&InThumbnailRenderer);
}

FMaterialEditorPanel::~FMaterialEditorPanel() = default;

void FMaterialEditorPanel::OpenMaterial(FAssetHandle MaterialHandle) {
    if (mRegistry.ResolveAsset<USurfaceOpaque>(MaterialHandle) == nullptr) {
        return;
    }

    mMaterialHandle = MaterialHandle;
    mPreviewDirty = true;
    SetVisible(true);
}

void FMaterialEditorPanel::RenderOffscreen(FRenderer& Renderer, FAssetRegistry&) {
    if (!mPreviewDirty || mRegistry.ResolveAsset<USurfaceOpaque>(mMaterialHandle) == nullptr) {
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
    USurfaceOpaque* Material{mRegistry.ResolveAsset<USurfaceOpaque>(mMaterialHandle)};

    if (Material == nullptr) {
        ImGui::TextDisabled("Material unavailable.");
        return;
    }

    const FAssetPath* AssetPath{mRegistry.GetAssetPath(mMaterialHandle)};

    ImGui::TextUnformatted(AssetPath != nullptr ? AssetPath->mPath.c_str() : Material->GetAssetName().c_str());

    const float PreviewWidth{std::min(ImGui::GetContentRegionAvail().x, 400.0f)};

    if (ID3D11ShaderResourceView* Preview{mPreviewSurface.GetShaderResourceView()}; Preview != nullptr) {
        ImGui::Image(ImTextureRef{reinterpret_cast<ImTextureID>(Preview)}, ImVec2{PreviewWidth, PreviewWidth});
    } else {
        ImGui::Dummy(ImVec2{PreviewWidth, PreviewWidth});
    }

    ImGui::Separator();

    const TArray<FMaterialGroup>& Groups{Material->GetGroups()};

    for (Uint32 GroupIndex{}; GroupIndex < Groups.size(); ++GroupIndex) {
        ImGui::PushID(static_cast<int>(GroupIndex));

        const FString GroupName{Groups[GroupIndex].mName.ToString()};

        if (ImGui::CollapsingHeader(GroupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            DrawGroup(*Material, GroupIndex, Groups[GroupIndex]);
        }

        ImGui::PopID();
    }
}

void FMaterialEditorPanel::DrawGroup(USurfaceOpaque& Material, Uint32 GroupIndex, const FMaterialGroup& Group) {
    FReflectedDetails::Draw(*FMaterialGroup::StaticStruct(), &Group, mPropertyEditor, [this, &Material, GroupIndex](const FProperty& Property, const FPropertyValue& Value) {
        ModifyGroup(Material, GroupIndex, [&Property, &Value](FMaterialGroup& Target) {
            Property.WriteContainer(&Target, Value);
        });
    });
}

void FMaterialEditorPanel::ModifyGroup(USurfaceOpaque& Material, Uint32 GroupIndex, const std::function<void(FMaterialGroup&)>& Modifier) {
    if (Material.ModifyGroup(GroupIndex, Modifier)) {
        mPreviewDirty = true;
    }
}
