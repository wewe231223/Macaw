/**
 * @file FViewerCamera.h
 * @brief 에셋 경계를 기준으로 Fit, 회전, 이동과 확대를 수행하는 궤도 카메라입니다.
 * @ingroup Viewer
 */

#pragma once

#include "Core/Math/FBox.h"
#include "Core/Math/FRotator.h"
#include "Engine/FSceneView.h"

namespace Viewer {

    /**
     * @brief 에셋 경계를 기준으로 Fit, 회전, 이동과 확대를 수행하는 궤도 카메라입니다.
     * @ingroup Viewer
     */
    class FViewerCamera {
    public:

        /**
         * @brief 기본 회전·중심·거리와 수직 시야각으로 궤도 카메라를 구성합니다.
         */
        FViewerCamera();

    public:

        /**
         * @brief AABB의 여덟 꼭짓점이 가로·세로 시야각 안에 들어오도록 궤도 중심과 거리를 계산합니다.
         * @param Bounds Fit할 대상의 월드 공간 AABB입니다.
         * @param Size 출력 너비·높이(픽셀)입니다.
         * @param Padding 경계에 적용할 1 이상의 여백 배율입니다.
         * @return 유한한 경계·출력·여백이면 true이며 실패 시 카메라를 유지합니다.
         */
        bool Fit(const Core::FBox& Bounds, const Core::FVector2& Size, float Padding = 1.1f);

        /**
         * @brief 궤도 중심·거리·시야각과 출력 종횡비로 뷰·투영 행렬을 구성합니다.
         * @param Size 출력 또는 표시 영역 크기(픽셀)입니다.
         * @return 출력 크기에 대응하는 장면 뷰입니다.
         */
        Engine::FSceneView CalcSceneView(const Core::FVector2& Size) const;

        /**
         * @brief 화면 이동량에 따라 중심 주위를 회전하고 상하 각도를 제한합니다.
         * @param Delta 이전 마우스 위치 대비 화면 이동량(픽셀)입니다.
         */
        void Orbit(const Core::FVector2& Delta);

        /**
         * @brief 카메라 거리와 출력 높이를 반영하여 궤도 중심을 화면 방향으로 이동합니다.
         * @param Delta 화면 기준 이동량(픽셀)입니다.
         * @param Height 카메라 이동 비율을 계산할 출력 높이(픽셀)입니다.
         */
        void Pan(const Core::FVector2& Delta, float Height);

        /**
         * @brief 휠 입력에 따라 거리를 지수 배율로 조절하고 경계 반지름 기준으로 제한합니다.
         * @param Delta 확대·축소 휠 입력량입니다.
         */
        void Zoom(float Delta);

        /**
         * @brief 모든 성분이 유한한 경우 카메라 회전각을 설정합니다.
         * @param Rotation 설정할 카메라 회전각(도)입니다.
         */
        void SetRotation(const Core::FRotator& Rotation);

        /**
         * @brief 유한한 수직 시야각을 1~170도로 제한하여 설정합니다.
         * @param FieldOfView 수직 시야각(도)입니다.
         */
        void SetFieldOfView(float FieldOfView);

        /**
         * @brief 궤도 카메라의 회전각을 반환합니다.
         * @return 도 단위 회전값입니다.
         */
        const Core::FRotator& GetRotation() const;

        /**
         * @brief 카메라가 바라보는 궤도 중심을 반환합니다.
         * @return 월드 공간 중심 좌표입니다.
         */
        const Core::FVector& GetCenter() const;

        /**
         * @brief 카메라 조작·클립 평면의 기준 반지름을 반환합니다.
         * @return 월드 단위 경계 반지름입니다.
         */
        float GetRadius() const;

        /**
         * @brief 궤도 중심까지의 카메라 거리를 반환합니다.
         * @return 월드 단위 거리입니다.
         */
        float GetDistance() const;

    private:

        /**
         * @brief 궤도 카메라의 회전각(도)입니다.
         */
        Core::FRotator mRotation{-20.0f, 45.0f, 0.0f};

        /**
         * @brief 카메라가 바라보고 회전하는 중심의 월드 좌표입니다.
         */
        Core::FVector mCenter{};

        /**
         * @brief 카메라 거리와 클립 평면 계산의 기준이 되는 경계 반지름입니다.
         */
        float mRadius{1.0f};

        /**
         * @brief 카메라와 궤도 중심 사이의 월드 단위 거리입니다.
         */
        float mDistance{3.0f};

        /**
         * @brief 수직 시야각(도)입니다.
         */
        float mFieldOfView{60.0f};
    };
}
