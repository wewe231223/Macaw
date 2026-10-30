/**
 * @file FGuid.h
 * @brief 128비트 GUID의 생성, 문자열 변환, 파싱, 비교 및 해시 기능을 정의한다.
 */

#pragma once

#include "Core/Common.h"

/**
 * @brief 네 개의 32비트 값으로 128비트 GUID를 보관한다.
 * @details 기본 초기화된 GUID는 모든 블록이 0이며 유효하지 않다. 문자열 표현과 해시는 네 블록을 mA, mB, mC, mD 순서로 사용한다.
 */
struct FGuid {
    /** @brief GUID를 구성하는 첫 번째 32비트 블록. */
    Uint32 mA{0};

    /** @brief GUID를 구성하는 두 번째 32비트 블록. */
    Uint32 mB{0};

    /** @brief GUID를 구성하는 세 번째 32비트 블록. */
    Uint32 mC{0};

    /** @brief GUID를 구성하는 네 번째 32비트 블록. */
    Uint32 mD{0};

    /**
     * @brief Windows의 CoCreateGuid()를 사용하여 새 GUID를 생성한다.
     * @return 생성된 GUID. 생성에 실패하면 모든 블록이 0인 유효하지 않은 GUID.
     */
    static FGuid NewGuid();

    /**
     * @brief GUID를 대문자 16진수와 하이픈으로 표현한다.
     * @return 8-4-4-4-12자리 형식의 36바이트 문자열.
     * @details 각 블록을 mA, mB, mC, mD 순서로 표현하며 중괄호는 포함하지 않는다.
     */
    FString ToString() const;

    /**
     * @brief 문자열에서 128비트 GUID를 읽어 현재 값을 갱신한다.
     * @param GuidString 파싱할 문자열. 하이픈은 건너뛰며 16진수 영문자의 대소문자는 구분하지 않는다.
     * @return 32개의 16진수를 읽으면 true. 그 전에 다른 문자가 나오거나 입력이 끝나면 false.
     * @details 성공 시 네 블록을 모두 갱신하고 실패 시 기존 값을 유지한다. 32개의 16진수를 읽은 뒤의 문자는 검사하지 않는다.
     * @note 모든 값이 0인 문자열도 파싱에 성공한다. GUID의 유효성은 IsValid()로 별도 확인한다.
     */
    bool Parse(const FString& GuidString);

    /**
     * @brief GUID에 0이 아닌 블록이 있는지 확인한다.
     * @return 네 블록 중 하나라도 0이 아니면 true, 모두 0이면 false.
     */
    bool IsValid() const;

    /**
     * @brief 네 GUID 블록을 결합한 해시 값을 계산한다.
     * @return mA, mB, mC, mD의 해시를 순서대로 결합한 값.
     */
    std::size_t GetHash() const noexcept;

    /**
     * @brief 두 GUID의 네 블록이 모두 같은지 비교한다.
     * @param Other 비교할 GUID.
     * @return 모든 블록이 각각 같으면 true, 하나라도 다르면 false.
     */
    bool operator==(const FGuid& Other) const;

    /**
     * @brief 두 GUID의 블록 중 다른 값이 있는지 비교한다.
     * @param Other 비교할 GUID.
     * @return 하나라도 다른 블록이 있으면 true, 모두 같으면 false.
     */
    bool operator!=(const FGuid& Other) const;
};

namespace std {
    /** @brief FGuid를 해시 기반 컨테이너의 키로 사용할 수 있도록 해시 함수를 제공한다. */
    template <>
    struct hash<FGuid> {
        /**
         * @brief GUID의 해시 값을 계산한다.
         * @param Guid 해시 값을 계산할 GUID.
         * @return Guid.GetHash()의 결과.
         */
        std::size_t operator()(const FGuid& Guid) const noexcept;
    };
}
