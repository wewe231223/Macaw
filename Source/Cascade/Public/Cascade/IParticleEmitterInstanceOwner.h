/**
 * @file IParticleEmitterInstanceOwner.h
 * @brief 인스턴스의 이름과 공간 변환을 제공하는 엔진 독립 소유자 인터페이스를 선언합니다.
 * @ingroup Cascade
 */

#pragma once

#include "Core/Types.h"

namespace Core {
    struct FTransform;
}

/**
 * @brief 파티클 시스템 정의와 모듈 실행 컨텍스트 및 인스턴스 저장소를 제공합니다.
 */
namespace Cascade {

    /**
     * @brief 엔진 컴포넌트 의존성 없이 인스턴스의 이름과 공간 변환을 제공하는 소유자 인터페이스입니다.
     * @note 소유자 객체는 이를 참조하는 EmitterInstance보다 오래 유지되어야 합니다.
     * @ingroup Cascade
     */
    class IParticleEmitterInstanceOwner {
    public:
        /**
         * @brief 인스턴스 소유자 인터페이스의 다형적 소멸을 지원합니다.
         */
        virtual ~IParticleEmitterInstanceOwner();

    public:
        /**
         * @brief 인스턴스가 사용할 소유자의 현재 공간 변환을 제공합니다.
         * @return 소유자가 수명을 유지하는 변환의 읽기 전용 참조입니다.
         */
        virtual const Core::FTransform& GetTransform() const = 0;
        /**
         * @brief 실행 인스턴스를 구분할 소유자의 이름을 제공합니다.
         * @return 컨텍스트의 이름 조회에 사용할 문자열입니다.
         */
        virtual Core::FString GetInstanceName() const = 0;
    };

}
