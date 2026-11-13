/**
 * @file IViewerContent.h
 * @brief 에셋별 준비, 갱신, 경계, 장면과 촬영 상태를 공통 뷰어에 제공하는 인터페이스입니다.
 * @ingroup Viewer
 */

#pragma once

#include "Viewer/FViewerTypes.h"
#include "Core/Math/FBox.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {
    class FSceneInterface;
    class FPrimitiveDrawInterface;
}

namespace Viewer {

    /**
     * @brief 에셋별 준비, 갱신, 경계, 장면과 촬영 상태를 공통 뷰어에 제공하는 인터페이스입니다.
     * @ingroup Viewer
     */
    class IViewerContent {
    public:

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~IViewerContent();

    public:

        /**
         * @brief 콘텐츠가 렌더링 또는 촬영 준비에 사용할 수 있는 상태인지 확인합니다.
         * @return 필요한 에셋과 표시 리소스가 준비되었으면 true입니다.
         */
        virtual bool IsReady() const = 0;

        /**
         * @brief 현재 콘텐츠의 월드 공간 경계를 제공합니다.
         * @return 콘텐츠 수명에 종속되는 AABB의 읽기 전용 참조입니다.
         */
        virtual const Core::FBox& GetBounds() const = 0;

        /**
         * @brief 렌더링에 사용할 독립 미리보기 장면을 반환합니다.
         * @return 콘텐츠 또는 월드가 소유하는 비소유 장면 포인터이며 없으면 nullptr입니다.
         */
        virtual Engine::FSceneInterface* GetScene() const = 0;

        /**
         * @brief 장면 렌더링 대신 직접 표시할 2D 텍스처를 조회합니다.
         * @return 외부 소유의 비소유 포인터이며 직접 표시 콘텐츠가 아니면 nullptr입니다.
         */
        virtual Asset::UTexture2D* GetTexture() const;

        /**
         * @brief 경과 시간만큼 콘텐츠를 갱신합니다.
         * @param DeltaTime 이전 갱신 이후 경과 시간(초)입니다.
         */
        virtual void Tick(float DeltaTime);

        /**
         * @brief 변경된 콘텐츠 상태를 렌더 장면과 경계에 반영합니다.
         */
        virtual void Synchronize();

        /**
         * @brief 콘텐츠의 본·관절 등 추가 표시 도형을 오버레이에 기록합니다.
         * @param Overlay 추가 도형을 기록할 오버레이입니다.
         * @param Radius 본 표시 크기의 기준 반지름입니다.
         */
        virtual void BuildOverlay(Engine::FPrimitiveDrawInterface& Overlay, float Radius) const;

        /**
         * @brief 콘텐츠의 기본 촬영 시각·시드·고정 간격을 제공합니다.
         * @return 촬영 준비에 전달할 상태 값입니다.
         */
        virtual Viewer::FViewerCaptureState GetCaptureState() const;

        /**
         * @brief 촬영 시각과 시뮬레이션 간격을 검증하고 콘텐츠를 촬영 상태로 준비합니다.
         * @param State 준비·저장·복원할 상태 데이터입니다.
         * @param Error 실패 원인을 기록할 출력 문자열입니다.
         * @return 준비 완료는 Ready, 추가 준비가 필요하면 Pending, 실패하면 Failed이며 Error에 원인을 기록합니다.
         */
        virtual Viewer::EViewerPrepareResult PrepareCapture(const Viewer::FViewerCaptureState& State, Core::FString& Error);
    };
}
