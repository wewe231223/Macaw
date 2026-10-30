#include "pch.h"
#include "Render/FMeshDrawState.h"
#include "Asset/UMaterial.h"
#include "Asset/UMesh.h"
#include "Asset/FAssetRegistry.h"

void BuildMeshBatchElements(const UMesh& Mesh, Uint32 LODLevel, TArray<FMeshBatchElement>& OutElements) {
    OutElements.clear();

    if (!Mesh.HasLOD(static_cast<int>(LODLevel))) {
        return;
    }

    const TArray<FMeshSection>& Sections{Mesh.GetSections(static_cast<int>(LODLevel))};

    if (Sections.empty()) {
        const Uint32 Count{Mesh.GetIndexCount(static_cast<int>(LODLevel))};

        if (Count != 0) {
            OutElements.push_back(FMeshBatchElement{0, Count, 0, Mesh.GetIndexCount(0)});
        }

        return;
    }

    for (const FMeshSection& Section : Sections) {
        if (Section.mIndexCount != 0) {
            OutElements.push_back(FMeshBatchElement{Section.mFirstIndex, Section.mIndexCount, Section.mMaterialSlotIndex, Section.mSourceIndexCount != 0 ? Section.mSourceIndexCount : Section.mIndexCount});
        }
    }
}

FAssetHandle ResolveMeshMaterial(const UMesh& Mesh, const TArray<FMeshMaterialBinding>& Overrides, Uint32 SlotIndex) {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr) {
        return {};
    }

    const FName Slot{Mesh.GetMaterialSlotName(SlotIndex)};

    for (const FMeshMaterialBinding& Override : Overrides) {
        if (Override.mSlot == Slot && Registry->ResolveAsset<UMaterial>(Override.mMaterial) != nullptr) {
            return Override.mMaterial;
        }
    }

    const FAssetHandle Material{Mesh.GetDefaultMaterial(SlotIndex)};

    return Registry->ResolveAsset<UMaterial>(Material) != nullptr ? Material : Registry->EnsureDefaultStaticMeshMaterial();
}

FAssetHandle SelectMaterialPipeline(const UMaterial& Material, bool Overlay) {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    if (Registry == nullptr) {
        return {};
    }

    if (Overlay) {
        return Registry->FindAsset(FAssetPath{"/Game/Pipeline/Gizmo.json"});
    }

    switch (Material.GetShader()) {
        case EMaterialShader::Base:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/Base"});
        case EMaterialShader::Textured:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/TexturedBase"});
        case EMaterialShader::BaseSolid:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/BaseSolid"});
        case EMaterialShader::Alternate:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/Alternate"});
        case EMaterialShader::AlternateSolid:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/AlternateSolid"});
        case EMaterialShader::Sky:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/SkyDome.json"});
        case EMaterialShader::Gizmo:
            return Registry->FindAsset(FAssetPath{"/Game/Pipeline/Gizmo.json"});
        case EMaterialShader::Surface: {
            const FMaterialChunkSignature Signature{Material.BuildChunkSignature()};
            bool Textured{};

            for (Uint8 Index{}; Index < Signature.mTextureFieldCount; ++Index) {
                Textured = Textured || static_cast<bool>(Signature.GetTextureHandle(Index));
            }

            return Registry->FindAsset(FAssetPath{Textured ? "/Game/Pipeline/TexturedBase" : "/Game/Pipeline/Base"});
        }
    }

    return {};
}

bool BuildMeshDrawState(const FMeshBatch& Mesh, const FMeshBatchElement& Element, const UMaterial& Material, const FMaterialBuffer& Materials, FMeshDrawState& OutState, Uint32& OutMaterialIndex, bool Overlay) {
    OutState = {};
    OutMaterialIndex = Materials.GetMaterialIndex(Material);

    if (Element.mIndexCount == 0 || OutMaterialIndex == UINT32_MAX) {
        return false;
    }

    OutState.mPipelineHandle = SelectMaterialPipeline(Material, Overlay);
    OutState.mMeshHandle = Mesh.mMeshHandle;
    OutState.mTextureSignature = Material.BuildChunkSignature();
    OutState.mBlendMode = Material.GetBlendMode();
    OutState.mSky = !Overlay && Material.GetShader() == EMaterialShader::Sky;
    OutState.mFirstIndex = Element.mFirstIndex;
    OutState.mIndexCount = Element.mIndexCount;
    OutState.mLODLevel = Mesh.mLODLevel;
    OutState.mOriginalIndexCount = Element.mOriginalIndexCount;
    OutState.mMaterialSlotIndex = Element.mMaterialSlotIndex;

    return static_cast<bool>(OutState.mPipelineHandle);
}
