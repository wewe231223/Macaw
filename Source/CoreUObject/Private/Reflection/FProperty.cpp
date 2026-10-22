/**
 * @file FProperty.cpp
 * @brief 타입이 지워진 프로퍼티의 읽기, 쓰기, 편집 및 직렬화 규약입니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/FProperty.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/FEnumValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

/**
 * @brief FProperty의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FProperty::FProperty(FString Name, EPropertyType Type, EPropertyFlags Flags, FReader Reader, FWriter Writer, const UStruct* Struct, TArray<FEnumValue> Values, FWriter Loader, const FNativeType* NativeType)
	: mName{std::move(Name)},
	  mType{Type},
	  mFlags{Flags},
	  mReader{Reader},
	  mWriter{Writer},
	  mLoader{Loader != nullptr ? Loader : Writer},
	  mStruct{Struct},
	  mEnumValues{std::move(Values)},
	  mNativeType{NativeType} {
}

/**
 * @brief 이름 또는 표시 문자열을 반환합니다.
 */
const FString& FProperty::GetName() const {
    return mName;
}

/**
 * @brief 프로퍼티 저장 타입을 반환합니다.
 */
EPropertyType FProperty::GetType() const {
    return mType;
}

/**
 * @brief 프로퍼티 값의 네이티브 타입 식별 메타데이터를 반환합니다.
 */
const FNativeType* FProperty::GetNativeType() const {
    return mNativeType;
}

/**
 * @brief 요청한 프로퍼티 플래그가 설정되어 있는지 검사합니다.
 */
bool FProperty::HasFlag(EPropertyFlags Flag) const {
    return (static_cast<Uint32>(mFlags) & static_cast<Uint32>(Flag)) != 0;
}

/**
 * @brief 복합 값의 구조체 메타데이터를 반환합니다.
 */
const UStruct* FProperty::GetStruct() const {
    return mStruct;
}

/**
 * @brief 열거값 이름과 정수 값 목록을 반환합니다.
 */
const TArray<FEnumValue>& FProperty::GetEnumValues() const {
    return mEnumValues;
}

/**
 * @brief 컨테이너에 등록된 읽기 함수를 호출하여 프로퍼티 값을 가져옵니다.
 */
FPropertyValue FProperty::ReadValue(const void* Container) const {
    return mReader(Container);
}

/**
 * @brief 값 타입과 쓰기 정책을 검사한 뒤 등록된 함수로 프로퍼티를 변경합니다.
 */
bool FProperty::WriteValue(void* Container, const FPropertyValue& Value) const {
    if (HasFlag(EPropertyFlags::ReadOnly)) {
        return false;
    }

    return WriteValue(Container, Value, mWriter);
}

/**
 * @brief 값 타입과 쓰기 정책을 검사한 뒤 등록된 함수로 프로퍼티를 변경합니다.
 */
bool FProperty::WriteValue(void* Container, const FPropertyValue& Value, FWriter Writer) const {
    if (Writer == nullptr || Container == nullptr) {
        return false;
    }

    if (mType == EPropertyType::Enum) {
        const Int32* Number{std::get_if<Int32>(&Value)};

        if (Number == nullptr || std::none_of(mEnumValues.begin(), mEnumValues.end(), [Number](const FEnumValue& Entry) {
            return Entry.mValue == *Number;
        })) {
            return false;
        }
    }

    return Writer(Container, Value);
}

/**
 * @brief 프로퍼티의 저장 정책과 타입에 맞추어 값을 직렬화합니다.
 */
void FProperty::SerializeValue(void* Container, FArchive& Archive) const {
    if (!HasFlag(EPropertyFlags::Serialize) || HasFlag(EPropertyFlags::Transient) || Archive.HasError() || (Archive.IsLoading() && !Archive.HasField(mName))) {
        return;
    }

    FPropertyValue Value{ReadValue(Container)};

    if (mStruct != nullptr) {
        if (Archive.BeginObject(mName)) {
            mStruct->SerializeProperties(GetPropertyData(Value), Archive);

            Archive.EndObject();
        }
    } else {
        std::visit([&Archive, this](auto& Typed) {
            if constexpr (!std::is_same_v<std::remove_cvref_t<decltype(Typed)>, const void*>) {
                Archive.Serialize(mName, Typed);
            } else {
                Archive.SetError("Cannot serialize opaque property: " + mName);
            }
        }, Value);
    }

    if (Archive.IsLoading() && !Archive.HasError() && !WriteValue(Container, Value, mLoader)) {
        Archive.SetError("Property validation failed: " + mName);
    }
}
