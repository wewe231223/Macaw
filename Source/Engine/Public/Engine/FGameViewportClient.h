/**
 * @file FGameViewportClient.h
 * @brief 실행 월드의 활성 카메라 선택과 입력·뷰포트 갱신에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/FViewportClient.h"
#include "Core/Input.h"
#include "Engine/FWorldHandle.h"
#include "Engine/FSceneView.h"

namespace Engine {

    class UEngine;

    /**
     * @brief 실행 월드의 활성 카메라 선택과 입력·뷰포트 갱신를 담당합니다.
     */
    class FGameViewportClient final : public Engine::FViewportClient {
    public:
        /**
         * @brief 전달받은 설정과 의존성으로 FGameViewportClient의 초기 상태를 구성합니다.
         * @param[in] Engine 월드 조회와 실행을 제공할 엔진의 비소유 주소입니다.
         */
        explicit FGameViewportClient(Engine::UEngine* Engine);

    public:
        /**
         * @brief 엔진에서 월드 핸들을 검증하고 실행 뷰포트의 대상을 교체합니다.
         * @param[in] World 클라이언트에 연결할 엔진 소유 월드의 세대 핸들입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool SetWorld(Engine::FWorldHandle World);

        /**
         * @brief 이 클라이언트가 그리는 월드의 핸들을 반환합니다.
         * @return 조회한 대상의 세대 핸들이며, 대상이 없으면 무효 핸들입니다.
         */
        Engine::FWorldHandle GetWorldHandle() const;

        /**
         * @brief 실행 월드의 활성 카메라로 뷰포트 크기에 맞는 장면 뷰를 계산합니다.
         * @param[in] Size 계산할 뷰포트의 가로·세로 픽셀 크기입니다.
         * @param[out] View 계산한 카메라 뷰·투영 상태를 받을 출력입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool CalcSceneView(const Core::FVector2& Size, Engine::FSceneView& View) const;

        /**
         * @brief 게임 뷰포트가 연결된 월드에 프레임 입력을 전달합니다.
         * @param[in] Input 키·마우스 상태를 조회할 공통 입력입니다.
         * @param[in] DeltaTime 프레임에서 진행할 초 단위 시간입니다.
         */
        void ProcessInput(const Core::FInput& Input, float DeltaTime);

        /**
         * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
         * @param[in] Viewport 출력 타깃과 크기를 제공하는 뷰포트입니다.
         */
        void Draw(Engine::FSceneViewport* Viewport) override;

    private:
        /**
         * @brief 월드 조회와 실행을 제공하는 엔진의 비소유 주소입니다.
         */
        Engine::UEngine* mEngine{};

        /**
         * @brief 클라이언트가 사용하는 실행 월드의 세대 핸들입니다.
         */
        Engine::FWorldHandle mWorld{};
    };

}
