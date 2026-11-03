/**
 * @file FSceneView.h
 * @brief 카메라 위치와 방향, 뷰·투영 행렬 및 뷰포트 크기입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"

namespace Core {
    struct FRay;
}

namespace Engine {

    /**
     * @brief 카메라 위치와 방향, 뷰·투영 행렬 및 뷰포트 크기입니다.
     * @ingroup Engine
     */
    struct FSceneView {
    public:
        /**
         * @brief 뷰포트 픽셀 좌표를 월드 공간의 정규화된 광선으로 역투영합니다.
         * @param[in] Position 뷰포트 픽셀 좌표입니다.
         * @param[out] Ray 계산한 월드 광선입니다.
         * @return 역투영 광선을 구했으면 true입니다.
         */
        bool Deproject(const Core::FVector2& Position, Core::FRay& Ray) const;

    public:
        /**
         * @brief 공간상의 위치입니다.
         */
        Core::FVector mLocation{};

        /**
         * @brief 카메라의 전방 방향입니다.
         */
        Core::FVector mForward{1.0f, 0.0f, 0.0f};

        /**
         * @brief 월드에서 뷰 공간으로 바꾸는 뷰 행렬입니다.
         */
        Core::FMatrix mView{};

        /**
         * @brief 원근 투영 행렬입니다.
         */
        Core::FMatrix mProjection{};

        /**
         * @brief 뷰와 투영을 합성한 행렬입니다.
         */
        Core::FMatrix mViewProjection{};

        /**
         * @brief 뷰포트의 가로·세로 픽셀 크기입니다.
         */
        Core::FVector2 mSize{};

        /**
         * @brief 수직 시야각입니다.
         */
        float mFieldOfView{60.0f};

        /**
         * @brief 정사영 카메라 여부입니다.
         */
        bool mOrthographic{};

        /**
         * @brief 정사영 가로 가시 범위의 월드 길이입니다.
         */
        float mOrthographicWidth{20.0f};
    };

}
