/**
 * @file UEnum.cpp
 * @brief 열거형의 이름과 정수 값 목록을 보관하는 메타데이터입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

namespace CoreUObject {

    /**
     * @brief UEnum의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    CoreUObject::UEnum::UEnum(Core::FString Name, std::size_t Size)
		: CoreUObject::UStruct{std::move(Name), Size} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    CoreUObject::UEnum::~UEnum() = default;

    /**
     * @brief 열거형 메타데이터에 이름과 정수 값 목록을 추가합니다.
     */
    void CoreUObject::UEnum::AddValues(std::initializer_list<CoreUObject::FEnumValue> Values) {
        if (!mValues.empty()) {
            Core::ErrorHandler::Fatal("Reflection", "Enum values are already registered: " + GetName());
        }

        for (const CoreUObject::FEnumValue& Value : Values) {
            if (std::any_of(mValues.begin(), mValues.end(), [&Value](const CoreUObject::FEnumValue& Existing) {
                return Existing.mName == Value.mName || Existing.mValue == Value.mValue;
            })) {
                Core::ErrorHandler::Fatal("Reflection", "Duplicate enum value: " + Value.mName);
            }

            mValues.push_back(Value);
        }
    }

    /**
     * @brief 열거형 이름과 정수 값 목록을 반환합니다.
     */
    const Core::TArray<CoreUObject::FEnumValue>& CoreUObject::UEnum::GetValues() const {
        return mValues;
    }

}
