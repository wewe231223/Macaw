/**
 * @file UObject.h
 * @brief GUID, 세대 핸들, 리플렉션과 직렬화를 제공하는 객체의 기반 클래스입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/Guid.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"

#include <new>

class FObjectRegistry;
class FArchive;
class FProperty;
class FObjectDuplicationContext;
struct FPropertyChangedEvent;

/**
 * @brief GUID, 세대 핸들, 리플렉션과 직렬화를 제공하는 객체의 기반 클래스입니다.
 * @ingroup CoreUObject
 */
class UObject {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UObject);

public:
    /**
     * @brief UObject의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    UObject();
    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    virtual ~UObject();
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    UObject(const UObject&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    UObject& operator=(const UObject&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    UObject(UObject&&) = delete;
    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    UObject& operator=(UObject&&) = delete;

public:
    /**
     * @brief 최상위 객체 타입의 정적 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectRootClass(UObject, EClassFlags::NoFactory);
    /**
     * @fn UClass* UObject::GetClass() const
     * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
     * @return 실제 클래스 메타데이터의 비소유 주소입니다.
     */

    /**
     * @brief 리플렉션 상속 관계를 기준으로 객체가 요청한 타입인지 검사합니다.
     * @param[in] Class Class에 적용할 입력 값입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsA(const UClass* Class) const;
    /**
     * @brief 리플렉션 상속 관계를 기준으로 객체가 요청한 타입인지 검사합니다.
     * @tparam T 적용할 네이티브 타입입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    template <typename T>
    bool IsA() const;

    /**
     * @brief 객체의 영속 식별자를 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FGuid& GetGuid() const;
    /**
     * @brief 레지스트리 슬롯과 세대 핸들을 반환합니다.
     * @return 레지스트리 슬롯과 세대 핸들입니다.
     */
    FObjectHandle GetHandle() const;

    /**
     * @brief 이름 또는 표시 문자열을 반환합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    const FString& GetName() const;
    /**
     * @brief 비어 있지 않은 이름을 검증하여 객체 이름을 변경합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool SetName(const FString& Name);
    /**
     * @brief 유효한 장면 GUID로 객체의 영속 식별자를 복원합니다.
     * @param[in] Guid 복원 또는 조회할 영속 GUID입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool RestoreGuid(const FGuid& Guid);

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
     */
    virtual void Serialize(FArchive& Archive);
    /**
     * @brief 복제 그래프 생성 후 사용자 참조를 복제본으로 재연결하는 확장 지점입니다.
     * @param[in] Source 이 객체에 대응하는 원본입니다.
     * @param[in] Context 모든 복제 객체의 비소유 대응표입니다.
     * @return 재연결에 성공하면 true이며 false이면 월드 복제 전체를 취소합니다.
     * @note 기본 구현은 클래스 일치 여부만 확인합니다. 참조 필드는 Context로 조회하여 연결하며 대상 객체의 소유권은 이전하지 않습니다.
     */
    virtual bool PostDuplicate(const UObject* Source, const FObjectDuplicationContext* Context);
    /**
     * @brief 프로퍼티 값을 편집하기 직전에 호출되는 확장 지점입니다.
     * @param[in] Property 읽기, 쓰기 또는 알림을 적용할 프로퍼티 메타데이터입니다.
     */
    virtual void PreEditChange(const FProperty* Property);
    /**
     * @brief 프로퍼티 값이 변경된 뒤 호출되는 확장 지점입니다.
     * @param[in] Event Event에 적용할 입력 값입니다.
     */
    virtual void PostEditChangeProperty(const FPropertyChangedEvent& Event);

    /**
     * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @return FMemory::Free()로 해제해야 하는 정렬된 할당 주소입니다.
     */
    static void* operator new(std::size_t Size);
    /**
     * @brief 객체 통계에 포함되는 정렬된 메모리를 할당합니다.
     * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
     * @param[in] Alignment 요청하는 바이트 정렬입니다.
     * @return FMemory::Free()로 해제해야 하는 정렬된 할당 주소입니다.
     */
    static void* operator new(std::size_t Size, std::align_val_t Alignment);
    /**
     * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
     * @param[in] Pointer 할당 또는 해제할 블록의 주소입니다.
     */
    static void operator delete(void* Pointer) noexcept;
    /**
     * @brief 객체 통계에 포함된 정렬 메모리를 해제합니다.
     * @param[in] Pointer 할당 또는 해제할 블록의 주소입니다.
     * @param[in] Alignment 요청하는 바이트 정렬입니다.
     */
    static void operator delete(void* Pointer, std::align_val_t Alignment) noexcept;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();
    /**
     * @brief SIMD 저장소를 사용하는 수학 연산 또는 객체 등록을 위해 내부 접근을 허용합니다.
     */
    friend class FObjectRegistry;

private:
    /**
     * @brief 장면 저장과 파일 내부 참조 복원에 사용하는 영속 식별자입니다.
     */
    FGuid mGuid{};
    ReflectAccessor(mGuid, GetGuid, RestoreGuid, "Id", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly);

    /**
     * @brief 레지스트리 슬롯과 세대 핸들입니다.
     */
    FObjectHandle mHandle{};
    ReflectProperty(mHandle, "Handle", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이름 또는 표시 문자열입니다.
     */
    FString mName{"Object"};
    ReflectAccessor(mName, GetName, SetName, "Name", EPropertyFlags::Edit | EPropertyFlags::Serialize);

};

#include "CoreUObject/UObjectTemplates.h"
