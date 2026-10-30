/**
 * @file FAllocationHeader.h
 * @brief 정렬된 할당 직전에 보관하는 원본 주소와 요청 크기입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <cstddef>

namespace Core {

    /**
     * @brief 정렬된 할당 직전에 보관하는 원본 주소와 요청 크기입니다.
     * @ingroup Core
     */
    struct FAllocationHeader {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FAllocationHeader);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FAllocationHeader);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 해제에 사용할 원본 힙 주소입니다.
         */
        void* mRaw{};
        ReflectProperty(mRaw, "Raw", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 크기입니다.
         */
        std::size_t mSize{};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
