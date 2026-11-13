/**
 * @file UAnimationAsset.h
 * @brief 모든 애니메이션 에셋의 공통 기반입니다.
 * @ingroup Asset
 */

#pragma once

#include "CoreUObject/UObject.h"
#include "UAnimationAsset.generated.h"

namespace Asset {

    /**
     * @brief 애니메이션 에셋의 공통 기반입니다.
     * @details 언리얼과 달리 스켈레톤을 참조하지 않습니다. 본 트랙은 이름으로 본을 가리키며, 사용하는 쪽이 자신의 스켈레톤에 매핑합니다.
     */
    UCLASS(Abstract)
    class UAnimationAsset : public CoreUObject::UObject {
        GENERATED_BODY()

    public:

        /**
         * @brief UAnimationAsset의 초기 상태와 의존성을 구성합니다.
         */
        UAnimationAsset() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UAnimationAsset() override = default;

    public:

        /**
         * @brief 재생 길이를 초 단위로 반환합니다.
         * @return 에셋의 재생 길이입니다.
         */
        virtual double GetPlayLength() const = 0;

        /**
         * @brief 재생에 필요한 데이터가 있는지 검사합니다.
         * @return 유효한 에셋이면 true입니다.
         */
        virtual bool IsValid() const = 0;
    };

}
