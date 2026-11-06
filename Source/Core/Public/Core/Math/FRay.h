/**
 * @file FRay.h
 * @brief 원점과 방향으로 정의하는 월드 공간 광선입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"
#include "FRay.generated.h"

namespace Core {

    /**
     * @brief 원점과 방향으로 정의하는 월드 공간 광선입니다.
     * @ingroup Core
     */
    USTRUCT()
    struct alignas(16) FRay {
        GENERATED_BODY()

    public:
        /**
         * @brief FRay의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FRay() = default;

        /**
         * @brief FRay의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Origin 광선의 시작점입니다.
         * @param[in] Direction 광선의 진행 방향입니다.
         */
        FRay(const Core::FVector& Origin, const Core::FVector& Direction);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FRay() = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 복사할 원본 값입니다.
         */
        FRay(const Core::FRay& Other) = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 복사할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FRay& operator=(const Core::FRay& Other) = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 이동할 원본 값입니다.
         */
        FRay(Core::FRay&& Other) noexcept = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 이동할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FRay& operator=(Core::FRay&& Other) noexcept = default;

    public:
        /**
         * @brief 광선 원점입니다.
         */
        UPROPERTY(Edit, Serialize)
        Core::FVector mOrigin{};

        /**
         * @brief 광선 진행 방향입니다.
         */
        UPROPERTY(Edit, Serialize)
        Core::FVector mDirection{1.0f, 0.0f, 0.0f};
    };

}
