#include "pch.h"
#include "Asset/UMaterial.h"

UMaterial::UMaterial() {
    InitializeClass(StaticClass());
}

void UMaterial::Finalize(const IAssetRegistry* Registry) {
}

EMaterialBlendMode UMaterial::GetBlendMode() const {
    return EMaterialBlendMode::Opaque;
}

EMaterialShader UMaterial::GetShader() const {
    return mShader;
}

void UMaterial::SetShader(EMaterialShader Shader) {
    if (mShader != Shader) {
        mShader = Shader;
        MarkGPUDataDirty();
    }
}

void UMaterial::MarkGPUDataDirty() {
    ++mRenderRevision;
}

Uint64 UMaterial::GetRenderRevision() const {
    return mRenderRevision;
}
