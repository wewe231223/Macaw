/**
 * @file FLightSceneProxy.cpp
 * @brief 렌더 장면에 복사하는 광원 상태와 에디터 아이콘의 비소유 참조입니다.
 * @ingroup Engine
 */

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
