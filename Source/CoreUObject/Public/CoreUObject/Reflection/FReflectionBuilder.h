/**
 * @file FReflectionBuilder.h
 * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace CoreUObject {

    class UStruct;
    class FProperty;

    /**
     * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
     * @ingroup CoreUObject
     */
    class FReflectionBuilder {
    public:
        /**
         * @brief FReflectionBuilder의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in,out] Owner 프로퍼티를 등록할 대상 타입 메타데이터입니다.
         */
        explicit FReflectionBuilder(CoreUObject::UStruct* Owner);

    public:
        /**
         * @brief 프로퍼티를 등록할 대상 타입 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        CoreUObject::UStruct* GetOwner() const;

        /**
         * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
         * @param[in] Property 등록할 프로퍼티입니다.
         */
        void AddProperty(std::unique_ptr<CoreUObject::FProperty> Property);

    private:
        /**
         * @brief 프로퍼티를 등록할 대상 타입 메타데이터입니다.
         */
        CoreUObject::UStruct* mOwner{};
    };

}
