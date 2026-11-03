/**
 * @file FArrayOps.h
 * @brief 타입이 지워진 TArray를 다루는 프로퍼티별 함수 테이블입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Types.h"

namespace CoreUObject {

    class FProperty;

    /**
     * @brief 타입이 지워진 TArray를 다루는 프로퍼티별 함수 테이블입니다.
     * @ingroup CoreUObject
     * @note 레지스트리에 타입으로 등록하지 않으므로 바이너리마다 인스턴스가 달라도 충돌하지 않습니다.
     */
    struct FArrayOps {
    public:
        /**
         * @brief 빈 작업용 배열을 생성합니다.
         */
        void* (*mCreate)();

        /**
         * @brief mCreate로 생성한 배열을 파괴합니다.
         */
        void (*mDestroy)(void* Array);

        /**
         * @brief 배열의 원소 수를 반환합니다.
         */
        Core::Uint32 (*mSize)(const void* Array);

        /**
         * @brief 배열의 원소 수를 변경합니다.
         */
        void (*mResize)(void* Array, Core::Uint32 Count);

        /**
         * @brief 원소의 주소를 반환합니다.
         */
        void* (*mAt)(void* Array, Core::Uint32 Index);

        /**
         * @brief 원소 주소를 컨테이너로 사용하는 프로퍼티입니다.
         */
        const CoreUObject::FProperty* mElement;
    };

}
