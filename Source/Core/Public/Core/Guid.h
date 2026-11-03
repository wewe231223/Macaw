/**
 * @file Guid.h
 * @brief GUID 생성, 파싱과 문자열 변환을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

namespace Core {

    /**
     * @brief 객체 식별에 사용하는 128비트 GUID와 문자열 변환을 제공합니다.
     * @ingroup Core
     */
    struct FGuid {
    public:
        /**
         * @brief 새로운 GUID를 생성합니다.
         */
        FGuid();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FGuid() = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 복사할 원본 값입니다.
         */
        FGuid(const Core::FGuid& Other) = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 복사할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FGuid& operator=(const Core::FGuid& Other) = default;

        /**
         * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
         * @param[in] Other 이동할 원본 값입니다.
         */
        FGuid(Core::FGuid&& Other) noexcept = default;

        /**
         * @brief 상대 값의 상태를 대입합니다.
         * @param[in] Other 이동할 원본 값입니다.
         * @return 갱신된 자기 자신(*this)의 참조입니다.
         */
        Core::FGuid& operator=(Core::FGuid&& Other) noexcept = default;

    public:
        /**
         * @brief 어떤 객체도 가리키지 않는 빈 GUID를 반환합니다.
         * @return IsValid()가 false인 GUID입니다.
         */
        static Core::FGuid Null();

        /**
         * @brief GUID 문자열을 검증하여 결과에 기록합니다.
         * @param[in] Text 파싱하거나 버퍼에 복사할 문자열입니다.
         * @param[out] Result 파싱한 GUID를 받을 출력 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         * @note 실패하면 Result를 유효한 GUID로 사용할 수 없습니다.
         */
        static bool Parse(const Core::FString& Text, Core::FGuid& Result);

        /**
         * @brief GUID를 정규 문자열 표현으로 변환합니다.
         * @return 변환된 문자열입니다.
         */
        Core::FString ToString() const;

        /**
         * @brief GUID가 모두 0인 무효 값이 아닌지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsValid() const;

        /**
         * @brief 16바이트 GUID 값이 모두 같은지 검사합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool operator==(const Core::FGuid& Other) const;

    public:
        /**
         * @brief GUID의 16바이트 값입니다.
         */
        std::array<Core::Uint8, 16> mBytes{};
    };

}
