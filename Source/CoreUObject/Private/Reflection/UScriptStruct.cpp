/**
 * @file UScriptStruct.cpp
 * @brief 일반 C++ 타입의 프로퍼티와 구조체 상속 관계를 기술합니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <utility>

/**
 * @brief UScriptStruct의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UScriptStruct::UScriptStruct(FString Name, std::size_t Size, const UScriptStruct* Parent)
	: UStruct{std::move(Name), Size},
	  mParent{Parent} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UScriptStruct::~UScriptStruct() = default;

/**
 * @brief 부모 구조체 메타데이터를 반환합니다.
 */
const UScriptStruct* UScriptStruct::GetSuperStruct() const {
    return mParent;
}

/**
 * @brief 자기 자신을 포함한 상속 체인에 지정한 부모 타입이 있는지 검사합니다.
 */
bool UScriptStruct::IsChildOf(const UScriptStruct* Parent) const {
    for (const UScriptStruct* Current{this}; Current != nullptr; Current = Current->mParent) {
        if (Current == Parent) {
            return true;
        }
    }

    return false;
}
