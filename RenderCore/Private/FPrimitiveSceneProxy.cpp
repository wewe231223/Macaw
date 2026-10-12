#include "pch.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

FPrimitiveSceneProxy::FPrimitiveSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform)
	: mComponentHandle(ComponentHandle),
	  mOwnerHandle(OwnerHandle),
	  mTransform(Transform) {
}

FPrimitiveSceneProxy::~FPrimitiveSceneProxy() = default;

FObjectHandle FPrimitiveSceneProxy::GetComponentHandle() const {
    return mComponentHandle;
}

FObjectHandle FPrimitiveSceneProxy::GetOwnerHandle() const {
    return mOwnerHandle;
}

const FPrimitiveTransform& FPrimitiveSceneProxy::GetTransform() const {
    return mTransform;
}

void FPrimitiveSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const {
}

void FPrimitiveSceneProxy::SetTransform(const FPrimitiveTransform& Transform) {
    mTransform = Transform;
}

void FPrimitiveSceneProxy::SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax) {
}

void FPrimitiveSceneProxy::SetUVTransforms(std::span<const FPrimitiveUVTransform> Transforms) {
    mUVTransforms.assign(Transforms.begin(), Transforms.end());
}

FUVTransform FPrimitiveSceneProxy::GetUVTransform(FName MaterialGroup) const {
    const FUVTransform* Default{nullptr};

    for (const FPrimitiveUVTransform& Entry : mUVTransforms) {
        if (Entry.mMaterialGroup == MaterialGroup) {
            return Entry.mTransform;
        }

        if (Entry.mMaterialGroup.IsNone()) {
            Default = &Entry.mTransform;
        }
    }

    return Default != nullptr ? *Default : FUVTransform{};
}

const FMeshSceneData* FPrimitiveSceneProxy::GetMeshData() const {
    return nullptr;
}

void FPrimitiveSceneProxy::GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const {
}
