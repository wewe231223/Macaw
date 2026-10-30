/**
 * @file FHeightFogSceneProxy.cpp
 * @brief 컴포넌트에서 렌더 장면으로 복사하는 안개 설정과 에디터 아이콘의 비소유 참조입니다.
 * @ingroup Engine
 */

#include "Engine/FHeightFogSceneProxy.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Core/Math/FTransform.h"

namespace Engine {

    Engine::FPrimitiveSceneProxy Engine::FHeightFogSceneProxy::CreateIconProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{};
        Proxy.mId = mId;
        Proxy.mWorld = Core::FTransform{mPosition, {}, {1.0f, 1.0f, 1.0f}}.ToMatrix();
        Proxy.mSprite = Engine::FDrawSprite{mIcon, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, true};

        return Proxy;
    }

}
