/**
 * @file UObjectTemplates.h
 * @brief UObject의 템플릿 타입 검사 함수 정의입니다.
 * @ingroup CoreUObject
 */

#pragma once

/**
 * @brief 리플렉션 상속 관계를 기준으로 객체가 요청한 타입인지 검사합니다.
 */
template <typename T>
bool UObject::IsA() const {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return IsA(T::StaticClass());
}
