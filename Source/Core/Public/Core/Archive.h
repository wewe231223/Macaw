/**
 * @file Archive.h
 * @brief 로드·저장 공통 아카이브와 복합 수학 값 직렬화를 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/GuidHash.h"
#include "Core/Math.h"

#include <string_view>

/**
 * @brief 아카이브가 값을 읽을지 쓸지를 지정합니다.
 * @ingroup Core
 */
enum class EArchiveMode {
    /**
     * @brief 저장된 값에서 메모리로 읽습니다.
     */
    Loading,
    /**
     * @brief 메모리 값을 아카이브에 씁니다.
     */
    Saving
};

/**
 * @brief 로드와 저장에 공통으로 사용하는 이름 기반 직렬화 인터페이스입니다.
 * @ingroup Core
 */
class FArchive {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FArchive);

public:
    /**
     * @brief FArchive의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Mode 로드·저장 또는 기즈모 동작 모드입니다.
     */
    explicit FArchive(EArchiveMode Mode);

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~FArchive();

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FArchive);

    /**
     * @brief 아카이브가 로드 모드인지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsLoading() const;

    /**
     * @brief 객체 GUID를 재매핑하는 복제용 아카이브인지 반환합니다.
     * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
     */
    bool IsDuplicating() const;

    /**
     * @brief 객체 복제에 사용할 GUID 대응표의 비소유 주소를 설정합니다.
     * @param[in] Remapping 원본 GUID와 복제 GUID 사이의 대응표입니다.
     */
    void SetObjectGuidRemapping(const TMap<FGuid, FGuid, FGuidHash>* Remapping);

    /**
     * @brief 객체 GUID를 직렬화하고 로드 시 복제용 GUID 재매핑을 적용합니다.
     * @param[in] Name 직렬화할 필드의 이름입니다.
     * @param[in,out] Value 읽거나 기록·변환할 값입니다.
     */
    void SerializeObjectGuid(std::string_view Name, FGuid& Value);

    /**
     * @brief 등록된 GUID 대응표로 객체 참조를 복제 대상의 GUID로 변환합니다.
     * @param[in,out] Value 읽거나 기록·변환할 값입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool RemapObjectReference(FGuid& Value);

    /**
     * @brief 아카이브에 오류 메시지가 기록되어 있는지 검사합니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool HasError() const;

    /**
     * @brief 처음 발생한 오류 메시지를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FString& GetError() const;

    /**
     * @brief 아카이브에 처음 발생한 오류 메시지를 보관합니다.
     * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
     */
    void SetError(const FString& Message);

    /**
     * @brief 현재 컨테이너에 지정한 이름의 필드가 있는지 검사합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool HasField(std::string_view Name) const = 0;

    /**
     * @brief 이름으로 지정한 객체 컨테이너에 진입합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool BeginObject(std::string_view Name) = 0;

    /**
     * @brief 현재 객체 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
     */
    virtual void EndObject() = 0;

    /**
     * @brief 배열 컨테이너에 진입하고 로드 시 원소 개수를 읽습니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Count 저장할 배열 길이이며 로드 시 읽어 온 배열 길이를 받습니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    virtual bool BeginArray(std::string_view Name, Uint32& Count) = 0;

    /**
     * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
     */
    virtual void EndArray() = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, bool& Value) = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, Int32& Value) = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, Uint32& Value) = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, Uint64& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, float& Value) = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    virtual void Serialize(std::string_view Name, FString& Value) = 0;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FGuid& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FVector2& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FVector& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FVector4& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FQuat& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FRotator& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FMatrix& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FTransform& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FBox& Value);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FRay& Value);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 현재 동작 모드입니다.
     */
    EArchiveMode mMode{};
    ReflectProperty(mMode, "Mode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 복제 시 원본 GUID를 새 GUID로 변환하는 비소유 대응표입니다.
     */
    const TMap<FGuid, FGuid, FGuidHash>* mObjectGuidRemapping{};
    ReflectProperty(mObjectGuidRemapping, "ObjectGuidRemapping", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 처음 발생한 오류 메시지입니다.
     */
    FString mError{};
    ReflectProperty(mError, "Error", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
