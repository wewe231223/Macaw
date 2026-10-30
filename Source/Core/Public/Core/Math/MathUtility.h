/**
 * @file MathUtility.h
 * @brief 각도 변환, 교차 검사와 화면 투영 유틸리티를 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Math/FVector2.h"
#include "Core/Math/FRay.h"
#include "Core/Math/FMatrix.h"

namespace Core {

    /**
     * @brief 각도 변환, 교차 검사와 투영을 위한 수학 유틸리티입니다.
     * @ingroup Core
     */
    namespace Math {
        /**
         * @brief 도 단위 각도를 라디안으로 변환합니다.
         * @param[in] Degrees 도 단위 각도입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        float ToRadians(float Degrees);

        /**
         * @brief 라디안 단위 각도를 도로 변환합니다.
         * @param[in] Radians 라디안 단위 각도입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        float ToDegrees(float Radians);

        /**
         * @brief 두 스칼라의 차이가 지정한 허용 오차 이내인지 검사합니다.
         * @param[in] Left 왼쪽 피연산자입니다.
         * @param[in] Right 오른쪽 피연산자입니다.
         * @param[in] Tolerance 같음 또는 유효성을 판단하는 허용 오차입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool NearlyEqual(float Left, float Right, float Tolerance = 0.0001f);

        /**
         * @brief 광선과 평면의 앞쪽 교점을 계산합니다.
         * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
         * @param[in] Point 기준 평면이나 거리 질의의 점입니다.
         * @param[in] Normal 평면 또는 표면 법선입니다.
         * @param[in,out] Hit 교차 성공 시 충돌 정보 또는 교점을 기록합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IntersectPlane(const Core::FRay& Ray, const Core::FVector& Point, const Core::FVector& Normal, Core::FVector& Hit);

        /**
         * @brief 광선과 삼각형의 교차를 검사하여 광선상의 거리를 구합니다.
         * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
         * @param[in] A 삼각형의 첫 번째 정점입니다.
         * @param[in] B 삼각형의 두 번째 정점입니다.
         * @param[in] C 삼각형의 세 번째 정점입니다.
         * @param[in,out] Distance 교차 성공 시 광선상의 거리를 기록합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IntersectTriangle(const Core::FRay& Ray, const Core::FVector& A, const Core::FVector& B, const Core::FVector& C, float& Distance);

        /**
         * @brief 월드 위치를 뷰포트 픽셀 좌표로 투영합니다.
         * @param[in] Position 변환하거나 투영할 위치입니다.
         * @param[in] ViewProjection 뷰와 투영을 합성한 행렬입니다.
         * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
         * @param[in,out] Screen 뷰포트 기준 픽셀 좌표입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Project(const Core::FVector& Position, const Core::FMatrix& ViewProjection, const Core::FVector2& Size, Core::FVector2& Screen);

        /**
         * @brief 뷰포트 픽셀 좌표에서 월드 공간 피킹 광선을 생성합니다.
         * @param[in] Screen 뷰포트 기준 픽셀 좌표입니다.
         * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
         * @param[in] ViewProjection 뷰와 투영을 합성한 행렬입니다.
         * @param[in,out] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Deproject(const Core::FVector2& Screen, const Core::FVector2& Size, const Core::FMatrix& ViewProjection, Core::FRay& Ray);

        /**
         * @brief 2차원 점에서 유한 선분까지의 최단 거리를 구합니다.
         * @param[in] Point 기준 평면이나 거리 질의의 점입니다.
         * @param[in] Start 선분 시작점입니다.
         * @param[in] End 선분 끝점입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        float DistanceToSegment(const Core::FVector2& Point, const Core::FVector2& Start, const Core::FVector2& End);
    }

}
