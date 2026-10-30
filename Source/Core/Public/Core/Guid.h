/**
 * @file Guid.h
 * @brief GUID 생성, 파싱과 문자열 변환을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

/**
 * @brief 객체 식별에 사용하는 128비트 GUID와 문자열 변환을 제공합니다.
 * @ingroup Core
 */
struct FGuid {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FGuid);

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
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FGuid(const FGuid& Other) = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FGuid& operator=(const FGuid& Other) = default;

    /**
     * @brief 상대 값의 상태로 객체를 복사하거나 이동 생성합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     */
    FGuid(FGuid&& Other) noexcept = default;

    /**
     * @brief 상대 값의 상태를 대입합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FGuid& operator=(FGuid&& Other) noexcept = default;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FGuid);

    /**
     * @brief GUID 문자열을 검증하여 결과에 기록합니다.
     * @param[in] Text 파싱하거나 버퍼에 복사할 문자열입니다.
     * @param[in,out] Result 변환 또는 파싱이 성공한 경우 결과를 기록합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     * @note 실패하면 Result를 유효한 GUID로 사용할 수 없습니다.
     */
    static bool Parse(const FString& Text, FGuid& Result);

    /**
     * @brief GUID를 정규 문자열 표현으로 변환합니다.
     * @return 변환된 문자열입니다.
     */
    FString ToString() const;

    /**
     * @brief GUID가 모두 0인 무효 값이 아닌지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsValid() const;

    /**
     * @brief 저장된 성분 또는 식별 상태가 정확히 같은지 검사합니다.
     * @param[in] Other 비교, 복사 또는 연산할 상대 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool operator==(const FGuid& Other) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 바이트 저장소 또는 사용량입니다.
     */
    std::array<Uint8, 16> mBytes{};
    ReflectProperty(mBytes, "Bytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
