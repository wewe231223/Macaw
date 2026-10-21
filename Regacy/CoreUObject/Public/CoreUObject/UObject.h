/**
 * @file UObject.h
 * @brief 리플렉션, 객체 식별, Outer 관계와 직렬화의 기반 타입을 선언합니다.
 */
#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "Core/Common.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "CoreUObject/FObjectHandle.h"

class FReflectionRegistry;
#include <cstddef>
#include <new>
#include <span>
#include <type_traits>

class FArchive;
class FPropertyValue;
class UFunction;
class UObject;

namespace UObjectSystem {
    /**
     * @brief 객체를 등록하고 슬롯 인덱스와 세대가 담긴 핸들을 반환합니다.
     * @param Object 등록할 객체입니다. 이미 등록된 객체는 기존 핸들을 반환합니다.
     * @return 등록 핸들입니다. null, 파괴 중인 객체, 클래스 기본 객체, GUID 또는 이름 충돌이면 무효 핸들입니다.
     * @note 이름이 없으면 같은 Outer 안에서 고유한 이름을 생성합니다. 객체의 소유권은 인수하지 않습니다.
     */
    FObjectHandle Register(UObject* Object);
    /**
     * @brief 객체와 핸들이 일치하면 등록을 해제하고 슬롯 세대를 증가시킵니다.
     * @param Object 등록을 해제할 객체입니다.
     * @param Handle 해당 객체의 현재 등록 핸들입니다.
     * @note 객체를 삭제하지 않습니다. 객체가 null이거나 핸들이 일치하지 않으면 아무 작업도 하지 않습니다.
     */
    void Unregister(UObject* Object, FObjectHandle Handle);
    /**
     * @brief 이름과 Outer를 바꾸고 등록 인덱스와 Inner 관계를 갱신합니다.
     * @param Object 변경할 객체입니다.
     * @param NewName 새 이름입니다. 등록 객체에서 None이면 고유한 이름을 생성합니다.
     * @param NewOuter 새 Outer입니다. nullptr이면 최상위로 옮깁니다.
     * @return 변경 가능하면 true입니다. 파괴 상태, Outer 순환, 변경 정책 또는 등록 이름 충돌로 거부되면 false입니다.
     * @note 실제 변경이 있으면 OnIdentityChanged를 호출합니다.
     */
    bool Rename(UObject* Object, FName NewName, UObject* NewOuter);
    /**
     * @brief 객체 GUID와 등록된 GUID 검색 인덱스를 갱신합니다.
     * @param Object 변경할 객체입니다.
     * @param Guid 복원할 유효한 GUID입니다.
     * @return 성공하면 true입니다. null 객체, 무효 GUID 또는 등록 객체의 GUID 충돌이면 false입니다.
     */
    bool RestoreGuid(UObject* Object, const FGuid& Guid);
    /**
     * @brief 직접 속한 Inner들을 최상위로 분리하고 Object의 기존 Outer 관계도 해제합니다.
     * @param Object 관계를 해제할 유효한 객체입니다.
     * @note 등록된 Inner는 최상위 이름 충돌을 피하도록 이름을 조정하며 객체를 삭제하지 않습니다.
     */
    void DetachInners(UObject* Object);
    /**
     * @brief 슬롯에 등록 객체가 있는지 확인하고 현재 세대의 핸들을 반환합니다.
     * @param Index 조회할 슬롯 인덱스입니다.
     * @param[out] Out 성공하면 현재 핸들, 실패하면 무효 핸들입니다.
     * @return 슬롯이 범위 안에 있고 비어 있지 않으면 true입니다.
     */
    bool TryGet(Uint32 Index, FObjectHandle& Out);
    /**
     * @brief 빈 슬롯을 포함한 객체 저장소의 전체 슬롯 수를 반환합니다.
     * @return 객체 순회에서 사용하는 끝 인덱스입니다.
     */
    Uint32 GetItemCount();
}

/**
 * @brief GUID, 이름, Outer와 클래스 메타데이터를 갖는 엔진 객체의 기반 클래스입니다.
 * @note 생성만으로 객체 저장소에 등록되지는 않습니다. Outer 관계는 객체의 메모리 소유권을 의미하지 않습니다.
 */
class UObject {
public:
    ReflectTypes(UObject);

public:
    /**
     * @brief 새 GUID를 생성하고 현재 생성 문맥의 클래스 정보를 설정합니다.
     * @note NewObject를 사용하면 파생 클래스의 생성 문맥이 적용됩니다.
     */
    UObject();
    /**
     * @brief 등록을 해제하고 직접 속한 Inner 객체와 기존 Outer 관계를 분리합니다.
     */
    virtual ~UObject();
    /**
     * @brief 객체 식별 정보의 복제를 방지하기 위해 복사 생성을 금지합니다.
     */
    UObject(const UObject&) = delete;
    /**
     * @brief 객체 식별 정보의 복제를 방지하기 위해 복사 대입을 금지합니다.
     */
    UObject& operator=(const UObject&) = delete;

public:
    /**
     * @brief 객체의 지속 식별 GUID를 반환합니다.
     * @return 내부 GUID의 상수 참조입니다.
     */
    const FGuid& GetGuid() const;
    /**
     * @brief 현재 등록 핸들을 반환합니다.
     * @return 미등록 상태이면 무효 핸들입니다.
     */
    FObjectHandle GetHandle() const;
    /**
     * @brief 저장된 GUID를 복원하고 등록 검색 인덱스를 갱신합니다.
     * @param InGuid 복원할 유효한 GUID입니다.
     * @return 성공하면 true입니다. 무효 GUID 또는 등록 객체의 GUID 충돌이면 false입니다.
     */
    bool RestoreGuid(const FGuid& InGuid);

    /**
     * @brief 현재 객체 이름을 반환합니다.
     * @return Outer 안에서 사용하는 객체 이름입니다.
     */
    FName GetName() const;
    /**
     * @brief 현재 Outer를 유지하면서 이름을 변경합니다.
     * @param InName 새 이름입니다.
     * @return Rename의 성공 여부입니다.
     */
    bool SetName(FName InName);
    /**
     * @brief 이름과 선택한 Outer를 함께 변경합니다.
     * @param NewName 새 이름입니다. 등록 객체에서 None이면 고유 이름을 생성합니다.
     * @param NewOuter 새 Outer입니다. nullptr이면 현재 Outer를 유지합니다.
     * @return 변경 정책과 등록 이름 검사를 통과하면 true입니다.
     * @note 최상위로 분리하려면 SetOuter(nullptr)를 사용합니다.
     */
    bool Rename(FName NewName, UObject* NewOuter = nullptr);

    /**
     * @brief 직접 포함하는 Outer를 반환합니다.
     * @return Outer 포인터입니다. 최상위 객체이면 nullptr입니다.
     */
    UObject* GetOuter() const;
    /**
     * @brief Outer 관계를 따라 최상위 객체를 찾습니다.
     * @return 최상위 객체입니다. Outer가 없으면 현재 객체입니다.
     */
    UObject* GetOutermost() const;
    /**
     * @brief Outer 관계에서 지정한 클래스와 호환되는 가장 가까운 객체를 찾습니다.
     * @param Type 검색할 클래스 메타데이터입니다.
     * @return 일치하는 Outer입니다. 없으면 nullptr입니다. 현재 객체 자체는 검사하지 않습니다.
     */
    UObject* GetTypedOuter(const UClass* Type) const;
    /**
     * @brief Outer 관계에서 T 또는 그 파생 타입인 가장 가까운 객체를 찾습니다.
     * @tparam T 자체 리플렉션 정보를 선언한 UObject 파생 타입입니다.
     * @return 일치하는 Outer입니다. 없으면 nullptr입니다.
     */
    template <typename T>
    T* GetTypedOuter() const;

    /**
     * @brief 지정한 객체가 현재 객체의 Outer 관계에 있는지 검사합니다.
     * @param Outer 찾을 Outer입니다.
     * @return 상위 Outer에 포함되거나 인수가 nullptr이면 true입니다. 현재 객체 자체는 포함 관계로 검사하지 않습니다.
     */
    bool IsIn(const UObject* Outer) const;
    /**
     * @brief 이름을 유지하며 직접 Outer를 변경합니다.
     * @param InOuter 새 Outer입니다. nullptr이면 최상위로 분리합니다.
     * @return 순환 관계, 변경 정책과 등록 이름 검사를 통과하면 true입니다.
     */
    bool SetOuter(UObject* InOuter);
    /**
     * @brief Outer 이름과 객체 이름을 점으로 연결한 경로를 만듭니다.
     * @param StopOuter 경로에 포함하지 않을 중단 Outer입니다. nullptr이면 최상위까지 포함합니다.
     * @return 상위에서 하위 순서로 연결한 경로입니다.
     */
    FString GetPathName(const UObject* StopOuter = nullptr) const;

    /**
     * @brief 저장 모드를 검사하고 가상 Serialize를 호출합니다.
     * @param Archive 저장 모드 아카이브입니다.
     */
    void Save(FArchive& Archive);
    /**
     * @brief 로드 모드를 검사하고 가상 Serialize를 호출합니다.
     * @param Archive 로드 모드 아카이브입니다.
     */
    void Load(FArchive& Archive);

    /**
     * @brief UObject 메모리 태그로 객체 저장 공간을 할당합니다.
     * @param Size 필요한 바이트 수입니다.
     * @return 기본 최대 정렬로 확보한 저장 공간입니다.
     */
    static void* operator new(std::size_t Size);
    /**
     * @brief 엔진 메모리 관리자로 객체 저장 공간을 해제합니다.
     * @param Ptr 해제할 저장 공간입니다.
     */
    static void operator delete(void* Ptr) noexcept;
    /**
     * @brief 지정한 정렬로 UObject 태그 저장 공간을 할당합니다.
     * @param Size 필요한 바이트 수입니다.
     * @param Alignment 필요한 정렬 바이트 수입니다.
     * @return 할당한 저장 공간입니다.
     */
    static void* operator new(std::size_t Size, std::align_val_t Alignment);
    /**
     * @brief 정렬 할당으로 확보한 객체 저장 공간을 해제합니다.
     * @param Ptr 해제할 저장 공간입니다.
     * @param Alignment 정렬 할당에 대응하는 인수입니다. 해제 구현은 이를 사용하지 않습니다.
     */
    static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;

    /**
     * @brief UObject의 기반 클래스 리플렉션 메타데이터를 반환합니다.
     * @return 등록된 UObject 클래스입니다.
     */
    ReflectRootClass(UObject, EClassFlags::None);
    /**
     * @brief 실제 객체 타입의 클래스 메타데이터를 반환합니다.
     * @return 클래스 정보입니다. 메타데이터 부트스트랩 도중에는 nullptr일 수 있습니다.
     */
    UClass* GetClass() const noexcept;
    /**
     * @brief 클래스 기본 객체로 생성되었는지 반환합니다.
     * @return 기본 객체이면 true입니다.
     */
    bool IsClassDefaultObject() const noexcept;
    /**
     * @brief 객체 클래스가 지정한 클래스 또는 그 파생 타입인지 검사합니다.
     * @param Class 비교할 클래스입니다.
     * @return 클래스가 있고 상속 관계가 일치하면 true입니다.
     */
    bool IsA(const UClass* Class) const noexcept;
    /**
     * @brief 객체가 T 또는 그 파생 타입인지 검사합니다.
     * @tparam T 자체 리플렉션 정보를 선언한 타입입니다.
     * @return 클래스 상속 관계가 일치하면 true입니다.
     */
    template <typename T>
    bool IsA() const noexcept;
    /**
     * @brief 현재 클래스와 기반 클래스에서 리플렉션 함수를 찾습니다.
     * @param Name 검색할 함수 이름입니다.
     * @return 함수 메타데이터입니다. 클래스가 없거나 이름을 찾지 못하면 nullptr입니다.
     */
    const UFunction* FindFunction(FName Name) const;
    /**
     * @brief 리플렉션 함수에 객체와 인수 목록을 전달합니다.
     * @param Function 호출할 함수입니다.
     * @param Arguments 함수 선언 순서의 인수 값입니다.
     * @param[out] Result 함수 반환 값입니다. null 함수나 호출 실패 시 비웁니다.
     * @return 함수의 객체·인수·반환 타입 검증과 호출이 성공하면 true입니다.
     */
    virtual bool ProcessEvent(const UFunction* Function, std::span<const FPropertyValue> Arguments, FPropertyValue& Result);

protected:
    /**
     * @brief 생성 문맥을 고려해 객체의 실제 클래스 정보를 설정합니다.
     * @param Class 기본으로 적용할 클래스입니다.
     * @note 생성 중인 클래스가 Class의 파생 타입이면 생성 문맥의 클래스를 유지합니다.
     */
    void InitializeClass(UClass* Class);

    /**
     * @brief GUID, 타입 이름, 객체 이름과 직렬화 대상 프로퍼티를 저장하거나 읽습니다.
     * @param Archive 저장 또는 로드 아카이브입니다.
     * @note 로드 오류가 없으면 GUID와 이름을 복원합니다. 참조된 타입 이름으로 객체를 재생성하지는 않습니다.
     */
    virtual void Serialize(FArchive& Archive);

private:
    /**
     * @brief 객체 저장소가 관리하는 등록 핸들을 설정합니다.
     * @param InHandle 새 핸들입니다.
     */
    void SetHandle(FObjectHandle InHandle);
    /**
     * @brief 파생 객체가 Outer 변경을 허용하는지 검사하는 확장 지점입니다.
     * @param NewOuter 변경할 Outer입니다.
     * @return 기본 구현은 true입니다.
     */
    virtual bool CanChangeOuter(const UObject* NewOuter) const;
    /**
     * @brief 이름 또는 Outer가 실제로 바뀐 뒤 호출되는 확장 지점입니다.
     */
    virtual void OnIdentityChanged();

    /**
     * @brief 메타데이터 부트스트랩 과정에서 객체 클래스 정보를 설정할 수 있게 합니다.
     */
    friend class FReflectionRegistry;
    /**
     * @brief 클래스 기본 객체의 상태를 관리할 수 있게 합니다.
     */
    friend class UClass;

    /**
     * @brief 객체 등록 함수가 내부 식별 상태와 핸들을 갱신할 수 있게 합니다.
     * @param Object 등록할 객체입니다.
     * @return 등록 핸들입니다. 등록 조건을 충족하지 못하면 무효 핸들입니다.
     */
    friend FObjectHandle UObjectSystem::Register(UObject* Object);
    /**
     * @brief 등록 해제 함수가 객체의 내부 핸들을 비울 수 있게 합니다.
     * @param Object 등록을 해제할 객체입니다.
     * @param Handle 현재 객체와 일치해야 하는 등록 핸들입니다.
     */
    friend void UObjectSystem::Unregister(UObject* Object, FObjectHandle Handle);
    /**
     * @brief 식별 변경 함수가 이름과 Outer·Inner 관계를 함께 갱신할 수 있게 합니다.
     * @param Object 변경할 객체입니다.
     * @param NewName 새 이름입니다.
     * @param NewOuter 새 Outer입니다. nullptr이면 최상위로 분리합니다.
     * @return 식별 변경에 성공하면 true입니다.
     */
    friend bool UObjectSystem::Rename(UObject* Object, FName NewName, UObject* NewOuter);
    /**
     * @brief GUID 복원 함수가 내부 GUID와 등록 인덱스를 함께 갱신할 수 있게 합니다.
     * @param Object 변경할 객체입니다.
     * @param Guid 복원할 GUID입니다.
     * @return GUID 복원이 허용되면 true입니다.
     */
    friend bool UObjectSystem::RestoreGuid(UObject* Object, const FGuid& Guid);
    /**
     * @brief 관계 해제 함수가 Inner 객체와 기존 Outer 연결을 정리할 수 있게 합니다.
     * @param Object 관계를 해제할 객체입니다.
     */
    friend void UObjectSystem::DetachInners(UObject* Object);

private:
    ReflectReadOnlyProperty(mClass, "Class", EPropertyFlags::ReadOnly | EPropertyFlags::Transient | EPropertyFlags::Reference);
    /**
     * @brief 실제 객체 타입의 클래스 메타데이터입니다.
     */
    UClass* mClass{};
    /**
     * @brief 클래스 기본 객체 여부입니다.
     */
    bool mClassDefaultObject{};
    ReflectReadOnlyProperty(mGuid, "Guid", EPropertyFlags::ReadOnly);
    /**
     * @brief 지속 객체 식별 GUID입니다.
     */
    FGuid mGuid{};
    /**
     * @brief 객체 저장소의 슬롯과 세대를 식별하는 등록 핸들입니다.
     */
    FObjectHandle mHandle{};
    ReflectReadOnlyProperty(mName, "Name", EPropertyFlags::ReadOnly);
    /**
     * @brief 직접 Outer 안에서 사용하는 이름입니다.
     */
    FName mName{};
    ReflectReadOnlyProperty(mOuter, "Outer", EPropertyFlags::ReadOnly | EPropertyFlags::Transient | EPropertyFlags::Reference);
    /**
     * @brief 직접 포함 관계를 나타내는 비소유 Outer 포인터입니다.
     */
    UObject* mOuter{};
    /**
     * @brief 현재 객체를 직접 Outer로 사용하는 비소유 객체 목록입니다.
     */
    TArray<UObject*> mInnerObjects{};
    /**
     * @brief 소멸 중 등록과 식별 변경을 차단하는 상태입니다.
     */
    bool mDestroying{};

    ReflectMembers();
};

template <typename T>
T* UObject::GetTypedOuter() const {
    static_assert(std::is_base_of_v<UObject, T>);
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return static_cast<T*>(GetTypedOuter(T::StaticClass()));
}

template <typename T>
bool UObject::IsA() const noexcept {
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return IsA(T::StaticClass());
}
