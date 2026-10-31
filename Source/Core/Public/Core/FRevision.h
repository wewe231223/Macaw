/**
 * @file FRevision.h
 * @brief 프로세스 전체에서 중복되지 않는 데이터 변경 번호를 정의합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Core {

    /**
     * @brief 프로세스 전체에서 중복되지 않는 데이터 변경 번호입니다.
     * @details 데이터 원본은 바뀔 때마다 Next로 새 번호를 받고, 캐시는 만들 때 본 번호와 현재 번호를 비교해 유효성을 판단합니다.
     * 모든 원본이 전역 카운터 하나를 공유하므로, 서로 다른 객체나 다시 만든 객체의 번호가 겹치지 않습니다.
     */
    class FRevision {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FRevision);

    public:
        /**
         * @brief 어떤 변경에도 해당하지 않는 무효 번호로 초기화합니다.
         */
        FRevision() = default;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FRevision);

        /**
         * @brief 전역 카운터에서 이전에 발급한 적 없는 새 번호를 발급합니다.
         * @return 새로 발급한 변경 번호입니다.
         */
        static Core::FRevision Next();

        /**
         * @brief 발급된 번호인지 검사합니다.
         * @return 기본 생성한 무효 번호가 아니면 true입니다.
         */
        bool IsValid() const;

        /**
         * @brief 번호의 정수 값을 반환합니다.
         * @return 무효 번호이면 0, 그 외에는 발급 순서대로 증가하는 값입니다.
         */
        Core::Uint64 GetValue() const;

        /**
         * @brief 두 번호가 같은 변경을 가리키는지 비교합니다.
         * @param[in] Other 비교할 변경 번호입니다.
         * @return 같은 번호이면 true입니다.
         */
        bool operator==(const Core::FRevision& Other) const = default;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 지정한 정수 값으로 번호를 만듭니다.
         * @param[in] Value 발급된 번호의 정수 값입니다.
         */
        explicit FRevision(Core::Uint64 Value);

    private:
        /**
         * @brief 발급 순서대로 증가하는 번호이며 0은 무효 번호입니다.
         */
        Core::Uint64 mValue{};
        ReflectProperty(mValue, "Value", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
