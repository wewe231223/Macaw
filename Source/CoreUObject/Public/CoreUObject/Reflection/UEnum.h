/**
 * @file UEnum.h
 * @brief 열거형의 이름과 정수 값 목록을 보관하는 메타데이터입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"

#include <initializer_list>

/**
 * @brief 열거형의 이름과 정수 값 목록을 보관하는 메타데이터입니다.
 * @ingroup CoreUObject
 */
class UEnum final : public UStruct {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UEnum, UStruct);

public:
    /**
     * @brief UEnum의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     */
    UEnum(FString Name, std::size_t Size);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~UEnum() override;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(UEnum);

    /**
     * @brief 열거형 메타데이터에 이름과 정수 값 목록을 추가합니다.
     * @param[in] Values 등록할 열거값 목록입니다.
     */
    void AddValues(std::initializer_list<FEnumValue> Values);
    /**
     * @brief 열거형 이름과 정수 값 목록을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const TArray<FEnumValue>& GetValues() const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 열거형 이름과 정수 값 목록입니다.
     */
    TArray<FEnumValue> mValues{};
    ReflectProperty(mValues, "Values", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
