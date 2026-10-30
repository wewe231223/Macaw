/**
 * @file TMemberPointerTraits.h
 * @brief 멤버 포인터에서 소유 타입과 값 타입을 추출합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(TMemberPointerTraits);

    public:
        /**
         * @brief 멤버 포인터가 소속된 C++ 타입입니다.
         */
        using FOwner = TOwner;

        /**
         * @brief 멤버 포인터가 가리키는 값의 C++ 타입입니다.
         */
        using FValue = TValue;

    public:
        /**
         * @brief 이 템플릿 특수화의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectTemplate(TMemberPointerTraits);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };

}

#include "TMemberPointerTraitsReflection.generated.h"
