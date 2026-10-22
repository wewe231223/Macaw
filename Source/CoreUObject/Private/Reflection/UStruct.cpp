/**
 * @file UStruct.cpp
 * @brief 네이티브 타입의 이름, 크기와 프로퍼티 메타데이터를 관리합니다.
 * @ingroup CoreUObject
 */

#include "CoreUObject/Reflection/UStruct.h"
#include "Core/Archive.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <utility>

/**
 * @brief UStruct의 초기 상태와 전달받은 의존성을 구성합니다.
 */
UStruct::UStruct(FString Name, std::size_t Size)
	: mName{std::move(Name)},
	  mSize{Size} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
UStruct::~UStruct() = default;

/**
 * @brief 이름 또는 표시 문자열을 반환합니다.
 */
const FString& UStruct::GetName() const {
    return mName;
}

/**
 * @brief 크기를 반환합니다.
 */
std::size_t UStruct::GetSize() const {
    return mSize;
}

/**
 * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
 */
void UStruct::AddProperty(FProperty Property) {
    if (mSealed || std::any_of(mDeclaredProperties.begin(), mDeclaredProperties.end(), [&Property](const FProperty& Existing) {
        return Existing.GetName() == Property.GetName();
    })) {
        ErrorHandler::Fatal("Reflection", "Duplicate or sealed property: " + Property.GetName());
    }

    mDeclaredProperties.push_back(std::move(Property));
}

/**
 * @brief 프로퍼티 등록을 마무리하고 조회용 목록을 확정합니다.
 */
void UStruct::Seal() {
    if (mSealed) {
        return;
    }

    for (const FProperty& Property : mDeclaredProperties) {
        mProperties.push_back(&Property);
    }

    mSealed = true;
}

/**
 * @brief 조회에 사용하는 프로퍼티 비소유 주소 목록을 반환합니다.
 */
const TArray<const FProperty*>& UStruct::GetProperties() const {
    return mProperties;
}

/**
 * @brief 타입에 선언된 프로퍼티를 이름으로 조회합니다.
 */
const FProperty* UStruct::FindProperty(const FString& Name) const {
    const auto Position{std::find_if(mProperties.begin(), mProperties.end(), [&Name](const FProperty* Property) {
        return Property->GetName() == Name;
    })};

    return Position != mProperties.end() ? *Position : nullptr;
}

/**
 * @brief 지정한 프로퍼티가 타입의 프로퍼티 목록에 속하는지 검사합니다.
 */
bool UStruct::ContainsProperty(const FProperty& Property) const {
    return std::find(mProperties.begin(), mProperties.end(), &Property) != mProperties.end();
}

/**
 * @brief 저장 가능한 선언 프로퍼티를 아카이브에 순서대로 전달합니다.
 */
void UStruct::SerializeProperties(void* Container, FArchive& Archive) const {
    for (const FProperty* Property : mProperties) {
        Property->SerializeValue(Container, Archive);
    }
}
