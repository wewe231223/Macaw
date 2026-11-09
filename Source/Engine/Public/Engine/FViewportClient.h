/**
 * @file FViewportClient.h
 * @brief 뷰포트별 입력과 장면 그리기를 구현하는 인터페이스입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/FHitProxy.h"

namespace Core {
    struct FVector2;
}

namespace Engine {

    class FSceneViewport;

    /**
     * @brief 뷰포트별 입력과 장면 그리기를 구현하는 인터페이스입니다.
     * @ingroup Engine
     */
    class FViewportClient {
    public:
        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FViewportClient();

    public:
        /**
         * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
         * @param[in,out] Viewport 출력 타깃과 크기를 제공하는 뷰포트입니다.
         */
        virtual void Draw(Engine::FSceneViewport* Viewport) = 0;

        /**
         * @brief 뷰포트 좌표에 대응하는 선택 정보를 반환합니다.
         * @param[in] Viewport 입력 좌표와 크기를 제공하는 비소유 뷰포트입니다.
         * @param[in] Position 뷰포트 기준 마우스 픽셀 좌표입니다.
         * @return 기본 구현은 선택 대상이 없는 빈 프록시를 반환합니다.
         * @note 좌표 범위 검사는 FSceneViewport::GetHitProxy()가 수행하며 실제 판정은 파생 클라이언트가 구현합니다.
         */
        virtual Engine::FHitProxy GetHitProxy(const Engine::FSceneViewport* Viewport, const Core::FVector2& Position) const;
    };

}
