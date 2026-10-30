/**
 * @file FNativeType.h
 * @brief 네이티브 타입의 크기, 정렬과 컴파일러 타입 식별 문자열입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>
#include <string_view>

namespace CoreUObject {

    /**
     * @brief 네이티브 타입의 크기, 정렬과 컴파일러 타입 식별 문자열입니다.
     * @ingroup CoreUObject
     */
    struct FNativeType {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FNativeType);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FNativeType);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 네이티브 타입의 바이트 크기입니다.
         */
        std::size_t mSize{};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 타입 또는 할당에 필요한 바이트 정렬입니다.
         */
        std::size_t mAlignment{};
        ReflectProperty(mAlignment, "Alignment", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 네이티브 타입 이름입니다.
         */
        std::string_view mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
