#include "pch.h"
#include "RenderCore/FLightSceneProxy.h"

FLightSceneProxy::FLightSceneProxy(FObjectHandle ComponentHandle, const FLightShaderParameters& Parameters)
	: mComponentHandle(ComponentHandle),
	  mParameters(Parameters) {
}

FObjectHandle FLightSceneProxy::GetComponentHandle() const {
    return mComponentHandle;
}

const FLightShaderParameters& FLightSceneProxy::GetShaderParameters() const {
    return mParameters;
}

void FLightSceneProxy::SetTransform(const FMatrix& World) {
    mParameters.mPosition = World.Translation();
    mParameters.mDirection = World.Forward();
}
