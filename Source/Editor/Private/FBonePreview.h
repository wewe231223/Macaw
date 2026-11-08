#pragma once

#include "Asset/Import/FMeshImportBone.h"
#include "Engine/EEditorMesh.h"

namespace Editor {
    struct FBoneShape {
        Engine::EEditorMesh mMesh{Engine::EEditorMesh::Count};
        Core::FMatrix mWorld{};
        Core::Uint32 mBone{};
    };

    Core::TArray<Editor::FBoneShape> BuildBoneShapes(const Core::TArray<Asset::FMeshImportBone>& Bones, const Core::TArray<Core::FMatrix>& Transforms, float Radius);
}
