/**
 * @file NativeReflection.h
 * @brief 네이티브 타입 판별과 프로퍼티 어댑터 등록 함수를 제공합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "CoreUObject/Reflection/FReflectionBuilder.h"
#include "CoreUObject/Reflection/TMemberPointerTraits.h"
#include "CoreUObject/Reflection/TNativeEnum.h"

#include <type_traits>
#include <functional>
#include <limits>

namespace CoreUObject {

    /**
     * @brief 템플릿 타입의 크기, 정렬과 식별 문자열을 갖는 정적 메타데이터를 반환합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    template <typename T>
    const CoreUObject::FNativeType* GetNativeType();

    /**
     * @brief C++ 타입을 지원하는 프로퍼티 저장 타입으로 분류합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @return C++ 타입을 지원하는 프로퍼티 저장 타입으로 분류합니다.입니다.
     */
    template <typename T>
    constexpr CoreUObject::EPropertyType GetNativePropertyType();

    /**
     * @brief 지원되는 복합 값 타입의 구조체 메타데이터를 조회합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    template <typename T>
    const CoreUObject::UStruct* GetNativeStruct();

    /**
     * @brief 네이티브 구조체와 선택적 부모의 메타데이터를 등록합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    template <typename T>
    CoreUObject::UScriptStruct* DefineNativeStruct(const char* Name);

    /**
     * @brief 타입이 생성한 멤버 등록 함수를 리플렉션 빌더로 실행합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @tparam TMetadata 프로퍼티를 등록할 메타데이터 타입입니다.
     * @param[in,out] Information 프로퍼티를 등록할 클래스 또는 구조체 메타데이터입니다.
     */
    template <typename T, typename TMetadata>
    void BuildNativeReflection(TMetadata* Information);

    /**
     * @brief 지정된 getter 또는 멤버 포인터로 실제 값을 읽습니다.
     * @tparam Member 등록할 멤버 포인터입니다.
     * @tparam Getter 선택적인 getter 포인터이며 nullptr이면 멤버를 직접 읽습니다.
     * @param[in] Object 대상 객체입니다.
     * @return getter 또는 멤버에서 읽은 값입니다.
     */
    template <auto Member, auto Getter = nullptr>
    decltype(auto) ReadMemberValue(const typename CoreUObject::TMemberPointerTraits<decltype(Member)>::FOwner* Object);

    /**
     * @brief 네이티브 멤버를 프로퍼티 variant의 대응 저장 타입으로 변환합니다.
     * @tparam Member 등록할 멤버 포인터입니다.
     * @tparam Getter 선택적인 getter 포인터이며 nullptr이면 멤버를 직접 읽습니다.
     * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
     * @return 프로퍼티 variant에 담을 수 있도록 변환한 값입니다.
     */
    template <auto Member, auto Getter = nullptr>
    CoreUObject::FPropertyValue ReadMemberProperty(const void* Container);

    /**
     * @brief variant 타입과 Uint8 범위를 검사한 뒤 setter 또는 멤버 포인터로 씁니다.
     * @tparam Member 등록할 멤버 포인터입니다.
     * @tparam Getter 선택적인 getter 포인터이며 nullptr이면 멤버를 직접 읽습니다.
     * @tparam Setter 선택적인 setter 포인터이며 nullptr이면 멤버에 직접 씁니다.
     * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
     * @param[in] Value 프로퍼티 타입에 대응하는 variant 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    template <auto Member, auto Getter = nullptr, auto Setter = nullptr>
    bool WriteMemberProperty(void* Container, const CoreUObject::FPropertyValue& Value);

    /**
     * @brief 네이티브 멤버의 타입, 플래그와 읽기·쓰기 함수를 등록합니다.
     * @tparam Member 등록할 멤버 포인터입니다.
     * @tparam Getter 선택적인 getter 포인터이며 nullptr이면 멤버를 직접 읽습니다.
     * @tparam Setter 선택적인 setter 포인터이며 nullptr이면 멤버에 직접 씁니다.
     * @param[in,out] Information 프로퍼티를 추가할 리플렉션 빌더입니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
     */
    template <auto Member, auto Getter, auto Setter>
    void RegisterProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags);

    /**
     * @brief 읽기 전용 프로퍼티와 로드에 사용하는 별도 쓰기 함수를 등록합니다.
     * @tparam Member 등록할 멤버 포인터입니다.
     * @tparam Getter 선택적인 getter 포인터이며 nullptr이면 멤버를 직접 읽습니다.
     * @param[in,out] Information 프로퍼티를 추가할 리플렉션 빌더입니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
     */
    template <auto Member, auto Getter>
    void RegisterReadOnlyProperty(CoreUObject::FReflectionBuilder& Information, Core::FString Name, Core::EPropertyFlags Flags);

}

#include "CoreUObject/Reflection/NativeReflectionTemplates.h"
