#include "Engine/FLightSceneProxy.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Core/Math/FTransform.h"

FPrimitiveSceneProxy FLightSceneProxy::CreateIconProxy() const {
    FPrimitiveSceneProxy Proxy{};
    Proxy.mId = mId;
    Proxy.mWorld = FTransform{mPosition, {}, {1.0f, 1.0f, 1.0f}}.ToMatrix();
    Proxy.mSprite = FDrawSprite{mIcon, {1.0f, 1.0f}, mColor, true};
    return Proxy;
}
