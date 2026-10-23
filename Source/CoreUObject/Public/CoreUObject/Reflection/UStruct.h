/**
 * @file UStruct.h
 * @brief 네이티브 타입의 이름, 크기와 프로퍼티 메타데이터를 관리합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FProperty.h"

/**
 * @brief 네이티브 타입의 이름, 크기와 프로퍼티 메타데이터를 관리합니다.
 * @ingroup CoreUObject
 */
class UStruct {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UStruct);

public:
    /**
     * @brief UStruct의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Size 네이티브 타입의 바이트 크기입니다.
     */
    UStruct(FString Name, std::size_t Size);
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~UStruct();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(UStruct);

    /**
     * @brief 이름 또는 표시 문자열을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FString& GetName() const;
    /**
     * @brief 크기를 반환합니다.
     * @return 크기입니다.
     */
    std::size_t GetSize() const;

    /**
     * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     */
    void AddProperty(FProperty Property);
    /**
     * @brief 프로퍼티 등록을 마무리하고 조회용 목록을 확정합니다.
     */
    virtual void Seal();

    /**
     * @brief 조회에 사용하는 프로퍼티 비소유 주소 목록을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const TArray<const FProperty*>& GetProperties() const;
    /**
     * @brief 타입에 선언된 프로퍼티를 이름으로 조회합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    const FProperty* FindProperty(const FString& Name) const;
    /**
     * @brief 지정한 프로퍼티가 타입의 프로퍼티 목록에 속하는지 검사합니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool ContainsProperty(const FProperty* Property) const;

    /**
     * @brief 저장 가능한 선언 프로퍼티를 아카이브에 순서대로 전달합니다.
     * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     */
    void SerializeProperties(void* Container, FArchive& Archive) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 이름 또는 표시 문자열입니다.
     */
    FString mName{};
    ReflectProperty(mName, "Name", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 크기입니다.
     */
    std::size_t mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 현재 타입이 소유한 프로퍼티 목록입니다.
     */
    TArray<FProperty> mDeclaredProperties{};
    ReflectProperty(mDeclaredProperties, "DeclaredProperties", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 조회에 사용하는 프로퍼티 비소유 주소 목록입니다.
     */
    TArray<const FProperty*> mProperties{};
    ReflectProperty(mProperties, "Properties", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프로퍼티 등록을 마쳤는지 여부입니다.
     */
    bool mSealed{};
    ReflectProperty(mSealed, "Sealed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
