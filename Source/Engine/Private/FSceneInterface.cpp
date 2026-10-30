/**
 * @file FSceneInterface.cpp
 * @brief 엔진이 렌더 장면의 프리미티브를 추가, 갱신, 제거하는 추상 인터페이스입니다.
 * @ingroup Engine
 */

#include "Engine/FSceneInterface.h"

namespace Engine {

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::FSceneInterface::~FSceneInterface() = default;

    void Engine::FSceneInterface::AddLight(const Engine::FLightSceneProxy&) {
    }

    void Engine::FSceneInterface::RemoveLight(const Core::FGuid&) {
    }

    void Engine::FSceneInterface::AddHeightFog(const Engine::FHeightFogSceneProxy&) {
    }

    void Engine::FSceneInterface::RemoveHeightFog(const Core::FGuid&) {
    }

}
