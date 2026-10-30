/**
 * @file UStruct.h
 * @brief 필드, 구조체와 열거형 메타데이터 및 프로퍼티 순회·직렬화를 제공합니다.
 */

#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/UObject.h"
#include "CoreUObject/Reflection/FProperty.h"

/**
 * @brief 이름을 갖는 리플렉션 메타데이터 객체의 기반 클래스입니다.
 */
class UField : public UObject {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UField;

    /**
     * @brief UField의 기반 객체 타입입니다.
     */
    using Super = UObject;

public:
    /**
     * @brief 필드 이름과 메타데이터 객체 이름을 설정합니다.
     * @param FieldName 필드 이름입니다.
     */
    explicit UField(FName FieldName);

    /**
     * @brief 필드 메타데이터를 해제합니다.
     */
    ~UField() override = default;

public:
    /**
     * @brief UField 클래스의 메타데이터를 반환합니다.
     * @return 일반 객체 팩터리를 제공하지 않는 메타데이터 클래스입니다.
     */
    ReflectClass(UField, EClassFlags::Metadata | EClassFlags::NoFactory);

    /**
     * @brief 선언된 필드 이름을 반환합니다.
     * @return 필드의 리플렉션 이름입니다.
     */
    FName GetFieldName() const;

private:
    /**
     * @brief 선언 시 설정한 메타데이터 필드 이름입니다.
     */
    FName mFieldName{};
};

/**
 * @brief 크기, 정렬, 기반 구조체와 프로퍼티 목록을 관리하는 리플렉션 타입입니다.
 */
class UStruct : public UField {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UStruct;

    /**
     * @brief UStruct의 기반 메타데이터 타입입니다.
     */
    using Super = UField;

public:
    /**
     * @brief 필드 이름과 네이티브 저장 크기·정렬을 설정합니다.
     * @param FieldName 구조체 이름입니다.
     * @param Size 네이티브 타입의 바이트 크기입니다.
     * @param Alignment 네이티브 타입의 바이트 정렬입니다.
     */
    explicit UStruct(FName FieldName, std::size_t Size = 0, std::size_t Alignment = 1);

    /**
     * @brief 소유한 프로퍼티와 구조체 메타데이터를 해제합니다.
     */
    ~UStruct() override = default;

public:
    /**
     * @brief UStruct 클래스의 메타데이터를 반환합니다.
     * @return UStruct의 리플렉션 클래스입니다.
     */
    ReflectClass(UStruct, EClassFlags::Metadata | EClassFlags::NoFactory);

    /**
     * @brief 직접 기반 구조체 메타데이터를 반환합니다.
     * @return 기반 타입입니다. 없으면 nullptr입니다.
     */
    UStruct* GetSuperStruct() const;

    /**
     * @brief 기반 구조체를 설정합니다.
     * @param SuperStruct 새 기반 타입입니다. nullptr이면 기반 관계를 해제합니다.
     * @note 상속 순환을 만드는 경우 Critical 오류를 보고합니다.
     */
    void SetSuperStruct(UStruct* SuperStruct);

    /**
     * @brief 현재 타입부터 기반 타입을 따라 상속 관계를 검사합니다.
     * @param BaseStruct 비교할 기반 타입입니다.
     * @return 현재 타입과 같거나 기반 체인에 포함되면 true입니다.
     */
    bool IsChildOf(const UStruct* BaseStruct) const;

    /**
     * @brief 네이티브 타입의 저장 크기를 반환합니다.
     * @return 바이트 단위 크기입니다.
     */
    std::size_t GetStructureSize() const;

    /**
     * @brief 네이티브 타입의 저장 정렬을 반환합니다.
     * @return 바이트 단위 정렬입니다.
     */
    std::size_t GetStructureAlignment() const;

    /**
     * @brief 직접 선언한 프로퍼티의 소유권을 인수합니다.
     * @param Property 소유 타입이 현재 구조체인 프로퍼티입니다.
     * @note 빈 프로퍼티, 소유 타입 불일치 또는 상속 범위의 이름 중복이면 Critical 오류를 보고합니다.
     */
    void AddProperty(std::unique_ptr<FProperty> Property);

    /**
     * @brief 직접 선언한 프로퍼티 목록을 반환합니다.
     * @return 소유 프로퍼티 배열의 상수 참조입니다.
     */
    const TArray<std::unique_ptr<FProperty>>& GetDeclaredProperties() const;

    /**
     * @brief 선택적으로 기반 타입을 포함해 프로퍼티 목록을 수집합니다.
     * @param IncludeInherited 기반 프로퍼티를 포함할지 여부입니다.
     * @return 기반 타입부터 현재 타입 순서의 비소유 프로퍼티 포인터 배열입니다.
     */
    TArray<const FProperty*> GetProperties(bool IncludeInherited = true) const;

    /**
     * @brief 현재 구조체부터 기반 구조체 순서로 프로퍼티를 찾습니다.
     * @param Name 검색할 프로퍼티 이름입니다.
     * @return 찾은 프로퍼티입니다. 없으면 nullptr입니다.
     */
    const FProperty* FindProperty(FName Name) const;

    /**
     * @brief Serialize 플래그가 있고 Transient가 아닌 프로퍼티를 스코프별로 직렬화합니다.
     * @param Container 해당 구조체 레이아웃의 저장 공간입니다.
     * @param Archive 사용할 아카이브입니다.
     * @param IncludeInherited 기반 프로퍼티를 포함할지 여부입니다.
     * @note 같은 직렬화 스코프의 프로퍼티를 묶고 비어 있지 않은 스코프에는 객체 범위를 생성합니다.
     */
    void SerializeProperties(void* Container, FArchive& Archive, bool IncludeInherited = true) const;

    /**
     * @brief 직접 선언한 직렬화 대상 프로퍼티만 처리합니다.
     * @param Container 해당 구조체 레이아웃의 저장 공간입니다.
     * @param Archive 사용할 아카이브입니다.
     */
    void SerializeDeclaredProperties(void* Container, FArchive& Archive) const;

    /**
     * @brief 기반 타입과 현재 타입의 프로퍼티에서 객체 참조를 방문합니다.
     * @param Container 해당 구조체 레이아웃의 저장 공간입니다.
     * @param Visitor 참조된 객체를 처리할 콜백입니다.
     */
    void VisitObjectReferences(const void* Container, const std::function<void(UObject*)>& Visitor) const;

private:
    /**
     * @brief 직접 기반 구조체의 비소유 메타데이터 포인터입니다.
     */
    UStruct* mSuperStruct{};

    /**
     * @brief 네이티브 타입의 바이트 크기입니다.
     */
    std::size_t mStructureSize{};

    /**
     * @brief 네이티브 타입의 바이트 정렬입니다.
     */
    std::size_t mStructureAlignment{1};

    /**
     * @brief 직접 선언한 프로퍼티를 등록 순서로 소유하는 목록입니다.
     */
    TArray<std::unique_ptr<FProperty>> mProperties{};
};

/**
 * @brief FNativeType과 연결된 네이티브 구조체의 리플렉션 메타데이터입니다.
 */
class UScriptStruct : public UStruct {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UScriptStruct;

    /**
     * @brief UScriptStruct의 기반 메타데이터 타입입니다.
     */
    using Super = UStruct;

public:
    /**
     * @brief 네이티브 타입 정보에서 구조체 크기와 정렬을 설정합니다.
     * @param FieldName 구조체 이름입니다.
     * @param ValueType 유효한 네이티브 타입 정보입니다.
     */
    UScriptStruct(FName FieldName, const FNativeType* ValueType);

    /**
     * @brief 구조체 메타데이터를 해제합니다.
     */
    ~UScriptStruct() override = default;

public:
    /**
     * @brief UScriptStruct 클래스의 메타데이터를 반환합니다.
     * @return UScriptStruct의 리플렉션 클래스입니다.
     */
    ReflectClass(UScriptStruct, EClassFlags::Metadata | EClassFlags::NoFactory);

    /**
     * @brief 연결된 네이티브 타입 정보를 반환합니다.
     * @return 구조체 값의 타입 정보입니다.
     */
    const FNativeType* GetValueType() const;

private:
    /**
     * @brief 크기, 정렬과 값 연산을 제공하는 네이티브 타입 정보입니다.
     */
    const FNativeType* mValueType{};
};

/**
 * @brief 열거형의 이름과 정수 값 대응을 보관하는 메타데이터입니다.
 */
class UEnum : public UField {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UEnum;

    /**
     * @brief UEnum의 기반 메타데이터 타입입니다.
     */
    using Super = UField;

public:
    /**
     * @brief 열거형 이름을 설정합니다.
     * @param FieldName 열거형 이름입니다.
     */
    explicit UEnum(FName FieldName);

    /**
     * @brief 열거형 메타데이터와 값 목록을 해제합니다.
     */
    ~UEnum() override = default;

public:
    /**
     * @brief UEnum 클래스의 메타데이터를 반환합니다.
     * @return UEnum의 리플렉션 클래스입니다.
     */
    ReflectClass(UEnum, EClassFlags::Metadata | EClassFlags::NoFactory);

    /**
     * @brief 열거형 이름과 정수 값을 등록합니다.
     * @param Name 열거형 항목 이름입니다.
     * @param Value 항목의 정수 값입니다.
     * @note 이름 중복이면 Critical 오류를 보고합니다. 같은 값에 다른 이름을 등록할 수 있습니다.
     */
    void AddValue(FName Name, Int64 Value);

    /**
     * @brief 이름에 해당하는 정수 값을 찾습니다.
     * @param Name 검색할 항목 이름입니다.
     * @return 찾은 값입니다. 없으면 std::nullopt입니다.
     */
    std::optional<Int64> FindValue(FName Name) const;

    /**
     * @brief 정수 값에 해당하는 첫 등록 이름을 찾습니다.
     * @param Value 검색할 정수 값입니다.
     * @return 찾은 이름입니다. 없으면 None입니다.
     */
    FName FindName(Int64 Value) const;

    /**
     * @brief 등록 순서의 이름·값 쌍 목록을 반환합니다.
     * @return 내부 값 목록의 상수 참조입니다.
     */
    const TArray<std::pair<FName, Int64>>& GetValues() const;

private:
    /**
     * @brief 등록 순서로 저장한 열거형 이름과 정수 값입니다.
     */
    TArray<std::pair<FName, Int64>> mValues{};
};
