/**
 * @file FSceneViewport.h
 * @brief 뷰포트 클라이언트와 렌더 타깃의 입력 및 그리기 호출을 연결합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"
#include "Engine/FHitProxy.h"

namespace RenderCore {
    class IRendererModule;
}

namespace Core {
    struct FVector2;
}

namespace Engine {

    class FViewportClient;
    class FRenderTarget;

    /**
     * @brief 뷰포트 클라이언트와 렌더 타깃의 입력 및 그리기 호출을 연결합니다.
     * @ingroup Engine
     */
    class FSceneViewport {
    public:
        /**
         * @brief FSceneViewport의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Renderer 뷰포트 및 엔진이 참조할 렌더러입니다.
         * @param[in,out] Client 입력과 그리기를 처리할 뷰포트 클라이언트입니다.
         */
        FSceneViewport(RenderCore::IRendererModule* Renderer, Engine::FViewportClient* Client);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FSceneViewport();

    public:
        /**
         * @brief 새 크기에 맞추어 렌더 타깃 리소스를 생성하거나 교체합니다.
         * @param[in] Width 렌더 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 렌더 타깃의 세로 픽셀 수입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Resize(Core::Uint32 Width, Core::Uint32 Height);

        /**
         * @brief 이 장면 뷰포트가 입력과 그리기를 전달할 클라이언트를 교체합니다.
         * @param[in] Client 입력과 그리기를 처리할 클라이언트의 비소유 주소입니다.
         */
        void SetClient(Engine::FViewportClient* Client);

        /**
         * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
         */
        void Draw();

        /**
         * @brief 좌표를 검증하고 연결된 클라이언트의 피킹 결과를 반환합니다.
         * @param[in] Position 컬러 타깃 기준 마우스 픽셀 좌표입니다.
         * @return 적중한 대상의 선택 정보이며 타깃이나 클라이언트가 없거나 좌표가 유한한 타깃 범위 안에 없으면 빈
         * 프록시입니다.
         */
        Engine::FHitProxy GetHitProxy(const Core::FVector2& Position) const;

        /**
         * @brief 컬러 렌더 타깃 뷰를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        Engine::FRenderTarget* GetRenderTarget() const;

        /**
         * @brief 크기를 반환합니다.
         * @return 크기입니다.
         */
        Core::FVector2 GetSize() const;

    private:
        /**
         * @brief 연결된 렌더러의 비소유 주소입니다.
         */
        RenderCore::IRendererModule* mRenderer{};

        /**
         * @brief 뷰포트 클라이언트의 비소유 주소입니다.
         */
        Engine::FViewportClient* mClient{};

        /**
         * @brief 뷰포트가 소유한 렌더 타깃입니다.
         */
        std::unique_ptr<Engine::FRenderTarget> mTarget{};
    };

}
