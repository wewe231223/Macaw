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

    /**
     * @brief 광원 프록시의 GUID로 렌더 장면에 광원을 추가하거나 교체합니다.
     */
    void Engine::FSceneInterface::AddLight(const Engine::FLightSceneProxy&) {
    }

    /**
     * @brief 지정한 GUID의 광원을 렌더 장면에서 제거합니다.
     */
    void Engine::FSceneInterface::RemoveLight(const Core::FGuid&) {
    }

    /**
     * @brief 컴포넌트 GUID를 키로 높이 안개 프록시를 렌더 장면에 추가하거나 교체합니다.
     */
    void Engine::FSceneInterface::AddHeightFog(const Engine::FHeightFogSceneProxy&) {
    }

    /**
     * @brief 지정한 GUID의 높이 안개 프록시를 렌더 장면에서 제거합니다.
     */
    void Engine::FSceneInterface::RemoveHeightFog(const Core::FGuid&) {
    }

    void Engine::FSceneInterface::AddDecal(const Engine::FDecalSceneProxy&) {
    }

    void Engine::FSceneInterface::RemoveDecal(const Core::FGuid&) {
    }

}
