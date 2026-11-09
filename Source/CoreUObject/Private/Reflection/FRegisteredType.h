/**
 * @file FRegisteredType.h
 * @brief 타입 정의와 소유 중인 리플렉션 메타데이터를 묶은 등록 항목입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "FReflectionDefinition.h"

namespace CoreUObject {

    /**
     * @brief 타입 정의와 소유 중인 리플렉션 메타데이터를 묶은 등록 항목입니다.
     * @ingroup CoreUObject
     */
    struct FRegisteredType {
    public:
        /**
         * @brief 중복 등록 검사에 사용하는 타입 정의입니다.
         */
        CoreUObject::FReflectionDefinition mDefinition{};

        /**
         * @brief 소유 중인 타입 메타데이터입니다.
         */
        std::unique_ptr<CoreUObject::UStruct> mInformation{};
    };

}
