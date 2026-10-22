/**
 * @file FReflectionBuilder.cpp
 * @brief 생성된 등록 코드가 소유 타입에 프로퍼티를 추가하는 진입점입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UStruct.h"

/**
 * @brief FReflectionBuilder의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FReflectionBuilder::FReflectionBuilder(UStruct& Owner)
	: mOwner{&Owner} {
}

/**
 * @brief 소유 객체 또는 메타데이터를 반환합니다.
 */
UStruct* FReflectionBuilder::GetOwner() const {
    return mOwner;
}

/**
 * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
 */
void FReflectionBuilder::AddProperty(std::unique_ptr<FProperty> Property) {
    mOwner->AddProperty(std::move(*Property));
}
