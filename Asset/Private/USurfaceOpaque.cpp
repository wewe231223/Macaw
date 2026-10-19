#include "pch.h"
#include "Asset/USurfaceOpaque.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstddef>

namespace {
    struct FSurfaceGPUData {
        FVector4 mDiffuseAndOpacity{1.0f, 1.0f, 1.0f, 1.0f};
        FVector4 mAmbientAndShininess{};
        FVector4 mSpecularAndRefractionIndex{};
        FVector4 mEmissiveAndSharpness{};
        FVector4 mTransmissionFilter{};
        Int32 mIlluminationModel{};
        Uint32 mDissolveHalo{};
        float mPadding0{};
        float mPadding1{};
        FVector4 mReserved1{};
        FVector4 mReserved2{};
    };

    static_assert(sizeof(FSurfaceGPUData) == MaterialGpuStride);
    static_assert(offsetof(FSurfaceGPUData, mIlluminationModel) == 80);
    static_assert(offsetof(FSurfaceGPUData, mDissolveHalo) == 84);

}

USurfaceOpaque::USurfaceOpaque() {
    InitializeClass(StaticClass());
}

const FSurfaceParameters& USurfaceOpaque::GetParameters() const {
    return mParameters;
}

bool USurfaceOpaque::SetParameters(const FSurfaceParameters& Parameters) {
    if (!std::isfinite(Parameters.mDiffuse.mW)) {
        return false;
    }

    mParameters = Parameters;
    mParameters.mDiffuse.mW = std::clamp(mParameters.mDiffuse.mW, 0.0f, 1.0f);
    MarkGPUDataDirty();
    return true;
}

void USurfaceOpaque::BuildGPUData(FMaterialGPUSlot& OutSlot) const {
    const FSurfaceParameters& Parameters{mParameters};
    FSurfaceGPUData Data{};

    Data.mDiffuseAndOpacity = Parameters.mDiffuse;
    Data.mAmbientAndShininess = FVector4{Parameters.mAmbient.mX, Parameters.mAmbient.mY, Parameters.mAmbient.mZ, Parameters.mShininess};
    Data.mSpecularAndRefractionIndex = FVector4{Parameters.mSpecular.mX, Parameters.mSpecular.mY, Parameters.mSpecular.mZ, Parameters.mRefractionIndex};
    Data.mEmissiveAndSharpness = FVector4{Parameters.mEmissive.mX, Parameters.mEmissive.mY, Parameters.mEmissive.mZ, Parameters.mSharpness};
    Data.mTransmissionFilter = FVector4{Parameters.mTransmissionFilter.mX, Parameters.mTransmissionFilter.mY, Parameters.mTransmissionFilter.mZ, Parameters.mOpacityTexture.GetTexture() ? 1.0f : 0.0f};
    Data.mIlluminationModel = Parameters.mIlluminationModel;
    Data.mDissolveHalo = Parameters.mBDissolveHalo ? 1u : 0u;

    std::memcpy(OutSlot.mData.data(), &Data, sizeof(Data));
}

FMaterialChunkSignature USurfaceOpaque::BuildChunkSignature() const {
    FMaterialChunkSignatureBuilder Builder{};

    const FSurfaceParameters& Parameters{mParameters};

    Builder.AddTexture(Parameters.mAmbientTexture.GetTexture());
    Builder.AddTexture(Parameters.mDiffuseTexture.GetTexture());
    Builder.AddTexture(Parameters.mSpecularTexture.GetTexture());
    Builder.AddTexture(Parameters.mEmissiveTexture.GetTexture());
    Builder.AddTexture(Parameters.mTransmissionTexture.GetTexture());
    Builder.AddTexture(Parameters.mShininessTexture.GetTexture());
    Builder.AddTexture(Parameters.mOpacityTexture.GetTexture());
    Builder.AddTexture(Parameters.mBumpTexture.GetTexture());
    Builder.AddTexture(Parameters.mNormalTexture.GetTexture());
    Builder.AddTexture(Parameters.mDisplacementTexture.GetTexture());
    Builder.AddTexture(Parameters.mDecalTexture.GetTexture());
    Builder.AddTexture(Parameters.mReflectionTexture.GetTexture());

    return Builder.Build();
}

EMaterialBlendMode USurfaceOpaque::GetBlendMode() const {
    return mParameters.mDiffuse.mW < 1.0f ? EMaterialBlendMode::Translucent : EMaterialBlendMode::Opaque;
}

void USurfaceOpaque::Finalize(const IAssetRegistry* Registry) {
    FMaterialTextureMap* Maps[]{&mParameters.mAmbientTexture, &mParameters.mDiffuseTexture, &mParameters.mSpecularTexture, &mParameters.mEmissiveTexture, &mParameters.mTransmissionTexture, &mParameters.mShininessTexture, &mParameters.mOpacityTexture, &mParameters.mBumpTexture, &mParameters.mNormalTexture, &mParameters.mDisplacementTexture, &mParameters.mDecalTexture, &mParameters.mReflectionTexture};

    for (FMaterialTextureMap* Map : Maps) {
        Map->Resolve(Registry);
    }

    MarkGPUDataDirty();
}
