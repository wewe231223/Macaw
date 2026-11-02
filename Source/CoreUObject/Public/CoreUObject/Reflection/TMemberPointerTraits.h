/**
 * @file TMemberPointerTraits.h
 * @brief 멤버 포인터에서 소유 타입과 값 타입을 추출합니다.
 * @ingroup CoreUObject
 */

#pragma once

namespace CoreUObject {

    /**
     * @brief 멤버 포인터에서 소유 타입과 값 타입을 추출합니다.
     * @tparam T 분석할 네이티브 타입입니다.
     */
    template <typename T>
    struct TMemberPointerTraits;

    /**
     * @brief 멤버 포인터에서 소유 타입과 값 타입을 추출합니다.
     * @ingroup CoreUObject
     * @tparam TValue 네이티브 타입의 템플릿 인수입니다.
     * @tparam TOwner 네이티브 타입의 템플릿 인수입니다.
     */
    template <typename TValue, typename TOwner>
    struct TMemberPointerTraits<TValue TOwner::*> {
    public:
        /**
         * @brief 멤버 포인터가 소속된 C++ 타입입니다.
         */
        using FOwner = TOwner;

        /**
         * @brief 멤버 포인터가 가리키는 값의 C++ 타입입니다.
         */
        using FValue = TValue;
    };

}
