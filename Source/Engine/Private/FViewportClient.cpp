/**
 * @file FViewportClient.cpp
 * @brief 뷰포트별 입력과 장면 그리기를 구현하는 인터페이스입니다.
 * @ingroup Engine
 */

#include "Engine/FViewportClient.h"

namespace Engine {

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Engine::FViewportClient::~FViewportClient() = default;

    /**
     * @brief 피킹을 구현하지 않은 클라이언트의 기본 빈 선택 정보를 반환합니다.
     */
    Engine::FHitProxy Engine::FViewportClient::GetHitProxy(const Engine::FSceneViewport*, const Core::FVector2&) const {
        return {};
    }

}
