/**
 * @file FSequenceBindingTarget.h
 * @brief 시퀀스 바인딩과 씬 객체의 연결에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"

namespace Engine {

    /**
     * @brief 시퀀스 바인딩과 씬 객체의 연결을 저장합니다.
     * @ingroup Engine
     */
    struct FSequenceBindingTarget {
    public:
        /**
         * @brief 리플렉션에서 사용할 소유 타입 별칭입니다.
         */
        ReflectTypes(FSequenceBindingTarget);

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FSequenceBindingTarget);

    private:
        /**
         * @brief 생성된 멤버 등록 함수의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 시퀀스 에셋 안의 바인딩 ID입니다.
         */
        Core::FGuid mBindingId{};
        ReflectProperty(mBindingId, "BindingId", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 연결할 액터 또는 공간 컴포넌트의 GUID입니다.
         */
        Core::FGuid mObjectId{};
        ReflectProperty(mObjectId, "ObjectId", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
