/**
 * @file FMessageTypeInfo.h
 * @brief 메시지 타입의 주소 기반 식별 정보와 메시지 선언 매크로를 정의한다.
 */
#pragma once

#include <string_view>

/**
 * @brief 타입 이름을 보관하고 객체 자신의 주소로 메시지 타입을 식별한다.
 * @details 이름이 같은 별도 타입 정보 객체도 서로 다른 타입으로 취급한다. 각 메시지 타입은 수명 동안 동일한 정적 타입 정보 객체를 사용해야 한다.
 */
struct FMessageTypeInfo {
    /**
     * @brief 진단과 표시용 타입 이름의 뷰. 참조하는 문자열은 타입 정보가 사용되는 동안 유효해야 한다.
     */
    std::string_view mTypeName{};

    /**
     * @brief 지정한 포인터가 현재 타입 정보 객체를 가리키는지 확인한다.
     * @param Type 비교할 타입 정보 포인터.
     * @return Type이 현재 객체의 주소와 같으면 true, nullptr을 포함한 다른 주소이면 false.
     */
    bool IsExactlyA(const FMessageTypeInfo* Type) const noexcept;
};

/**
 * @brief 메시지 타입의 기본 특수 멤버 함수와 정적 타입 정보 조회 함수를 선언한다.
 * @param MessageType 매크로를 사용할 메시지 클래스 또는 구조체의 이름.
 * @details 기본 생성, 소멸, 복사 및 이동 연산을 기본 정의하며 이동 연산은 noexcept로 선언한다. public StaticTypeInfo()의 선언도 추가한다.
 * @note StaticTypeInfo()는 소스 파일에서 정의하고 해당 타입에 고유한 정적 FMessageTypeInfo 객체의 참조를 반환해야 한다.
 */
#define DECLARE_CHANNEL_MESSAGE(MessageType) \
    MessageType() = default; \
    ~MessageType() = default; \
    MessageType(const MessageType&) = default; \
    MessageType& operator=(const MessageType&) = default; \
    MessageType(MessageType&&) noexcept = default; \
    MessageType& operator=(MessageType&&) noexcept = default; \
public: \
    static const FMessageTypeInfo& StaticTypeInfo() noexcept
