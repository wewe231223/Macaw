/**
 * @file FReflectionBuilder.cpp
 * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"

namespace CoreUObject {

    /**
     * @brief FReflectionBuilder의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    CoreUObject::FReflectionBuilder::FReflectionBuilder(CoreUObject::UStruct* Owner)
		: mOwner{Owner} {
    }

    /**
     * @brief 프로퍼티를 등록할 대상 타입 메타데이터를 반환합니다.
     */
    CoreUObject::UStruct* CoreUObject::FReflectionBuilder::GetOwner() const {
        return mOwner;
    }

    /**
     * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
     */
    void CoreUObject::FReflectionBuilder::AddProperty(std::unique_ptr<CoreUObject::FProperty> Property) {
        mOwner->AddProperty(std::move(*Property));
    }

}
