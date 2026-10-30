/**
 * @file FName.h
 * @brief 대소문자 구분 없는 공유 이름 식별자입니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

#include <memory>
#include <string_view>

namespace Core {

    /**
     * @brief 같은 ASCII 대소문자 구분 없는 이름을 공유하여 포인터로 비교하는 식별자입니다.
     */
    class FName {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FName);

    public:
        /**
         * @brief 이름이 없는 식별자를 생성합니다.
         */
        FName() = default;

        /**
         * @brief 입력 이름을 ASCII 대소문자 구분 없이 공유 이름 풀에 등록합니다.
         * @param[in] Name 조회하거나 설정할 이름입니다.
         */
        explicit FName(std::string_view Name);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FName);

        /**
         * @brief 공유 이름의 동일성으로 대소문자를 구분하지 않고 비교합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool operator==(const Core::FName& Other) const;

        /**
         * @brief 두 공유 이름 식별자가 서로 다른지 비교합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool operator!=(const Core::FName& Other) const;

        /**
         * @brief 공유 이름이 없는 기본 식별자인지 검사합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsNone() const;

        /**
         * @brief 이 이름이 처음 등록되었을 때의 표기 문자열을 반환합니다.
         * @return 공유 이름 풀이 보관하는 문자열의 참조이며, None이면 빈 문자열입니다.
         */
        const Core::FString& ToString() const;

        /**
         * @brief 동등한 이름에 같은 값을 부여하는 프로세스 내부 해시를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        std::size_t GetHash() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 이름 풀에서 공유하는 최초 표기 문자열입니다.
         */
        std::shared_ptr<const Core::FString> mValue{};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 이름 비교와 같은 공유 문자열 주소를 해시하는 컨테이너 해시입니다.
     */
    struct FNameHash {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FNameHash);

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FNameHash);

        /**
         * @brief FName 동등 비교와 일치하는 해시를 반환합니다.
         * @param[in] Name 조회하거나 설정할 이름입니다.
         * @return 계산하거나 조회한 값입니다.
         */
        std::size_t operator()(const Core::FName& Name) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
