/**
 * @file FBox.h
 * @brief 최솟값과 최댓값으로 정의하는 축 정렬 경계 상자입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "FBox.generated.h"

namespace Core {

    struct FRay;
    struct FMatrix;

    /**
     * @brief 최솟값과 최댓값으로 정의하는 축 정렬 경계 상자입니다.
     * @ingroup Core
     */
    USTRUCT()
    struct alignas(16) FBox {
        GENERATED_BODY()

    public:
        /**
         * @brief FBox의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FBox() = default;

        /**
         * @brief FBox의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Minimum 성분별 하한입니다.
         * @param[in] Maximum 성분별 상한입니다.
         */
        FBox(const Core::FVector& Minimum, const Core::FVector& Maximum);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FBox() = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 복사할 원본 값입니다.
         */
        FBox(const Core::FBox& Other) = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 복사할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FBox& operator=(const Core::FBox& Other) = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 이동할 원본 값입니다.
         */
        FBox(Core::FBox&& Other) noexcept = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 이동할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FBox& operator=(Core::FBox&& Other) noexcept = default;

    public:
        /**
         * @brief 광선이 최대 거리 이내에서 축 정렬 경계 상자와 교차하는지 검사합니다.
         * @param[in] Ray 교차를 검사하거나 역투영 결과를 기록할 광선입니다.
         * @param[in] MaximumDistance 허용할 최대 광선 거리입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Intersects(const Core::FRay& Ray, float MaximumDistance) const;

        /**
         * @brief 경계 상자의 모서리를 변환한 뒤 새 축 정렬 경계 상자를 계산합니다.
         * @param[in] Matrix 적용할 변환 행렬입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        Core::FBox Transformed(const Core::FMatrix& Matrix) const;

    public:
        /**
         * @brief 축별 최솟값입니다.
         */
        UPROPERTY(Edit, Serialize)
        Core::FVector mMinimum{};

        /**
         * @brief 축별 최댓값입니다.
         */
        UPROPERTY(Edit, Serialize)
        Core::FVector mMaximum{};
    };

}
