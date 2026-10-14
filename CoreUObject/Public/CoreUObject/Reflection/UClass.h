/**
 * @file UClass.h
 * @brief UObject 클래스의 생성 정책, 기본 객체와 리플렉션 함수 메타데이터를 제공합니다.
 */
#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/EClassFlags.h"
#include <mutex>

class UFunction;

/**
 * @brief UClass::CreateInstance에 전달하는 객체 식별·등록 설정입니다.
 */
struct FObjectConstructionParameters {
    /**
     * @brief 새 객체에 설정할 Outer입니다. 기본값 nullptr는 최상위 객체를 나타냅니다.
     */
    UObject* mOuter{};
    /**
     * @brief 선택적으로 지정할 객체 이름입니다. None이면 생성 상태의 이름을 유지합니다.
     */
    FName mName{};
    /**
     * @brief 생성 후 객체 저장소에 등록할지 여부입니다. 기본값은 false입니다.
     */
    bool mRegister{};
};

/**
 * @brief 클래스 상속, 생성 팩터리, 기본 객체와 함수 목록을 관리하는 메타데이터입니다.
 */
class UClass : public UStruct {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UClass;
    /**
     * @brief 클래스 메타데이터의 기반 타입입니다.
     */
    using Super = UStruct;
    /**
     * @brief UStruct 기반의 상속 검사 오버로드도 공개합니다.
     */
    using UStruct::IsChildOf;
    /**
     * @brief 객체 소유권을 반환하는 무인수 생성 팩터리 타입입니다.
     */
    using FFactory = std::unique_ptr<UObject> (*)();

public:
    /**
     * @brief 클래스의 이름, 저장 규격과 생성 정책을 설정합니다.
     * @param ClassName 클래스 이름입니다.
     * @param Size 객체 타입의 바이트 크기입니다.
     * @param Alignment 객체 타입의 바이트 정렬입니다.
     * @param Flags 생성 정책 플래그입니다.
     * @param Factory 객체 생성 함수입니다. nullptr이면 생성 팩터리가 없습니다.
     */
    UClass(FName ClassName, std::size_t Size, std::size_t Alignment, EClassFlags Flags, FFactory Factory);
    /**
     * @brief 기본 객체와 직접 등록한 함수 메타데이터를 해제합니다.
     */
    ~UClass() override;

public:
    /**
     * @brief UClass 자체의 리플렉션 클래스 메타데이터를 반환합니다.
     * @return 메타데이터 클래스입니다.
     */
    ReflectClass(UClass, EClassFlags::Metadata | EClassFlags::NoFactory);
    /**
     * @brief 클래스의 리플렉션 이름을 반환합니다.
     * @return 필드 이름과 같은 클래스 이름입니다.
     */
    FName GetClassName() const;
    /**
     * @brief 직접 기반 클래스 정보를 반환합니다.
     * @return 기반 클래스입니다. 없으면 nullptr입니다.
     */
    UClass* GetSuperClass() const;
    /**
     * @brief 현재 클래스가 지정한 클래스 또는 그 파생 클래스인지 검사합니다.
     * @param BaseClass 비교할 클래스입니다.
     * @return 현재 클래스나 기반 클래스 체인에 일치하는 타입이 있으면 true입니다.
     */
    bool IsChildOf(const UClass* BaseClass) const;
    /**
     * @brief 현재 클래스와 T의 상속 관계를 검사합니다.
     * @tparam T 자체 리플렉션 정보를 선언한 클래스 타입입니다.
     * @return T와 같거나 T의 파생 클래스이면 true입니다.
     */
    template <typename T>
    bool IsChildOf() const;
    /**
     * @brief 지정한 플래그 중 하나라도 설정되어 있는지 검사합니다.
     * @param Flags 검사할 비트 마스크입니다.
     * @return 겹치는 비트가 있으면 true입니다.
     */
    bool HasAnyFlags(EClassFlags Flags) const;
    /**
     * @brief 일반 객체 생성 팩터리와 클래스 정책을 검사합니다.
     * @return 팩터리가 있고 Abstract, NoFactory, Metadata가 모두 없으면 true입니다.
     * @note 팩터리 호출 자체의 성공을 보장하지 않습니다.
     */
    bool CanCreateInstance() const;
    /**
     * @brief 팩터리로 객체를 생성하고 Outer, 이름과 선택적 등록을 적용합니다.
     * @param Parameters 새 객체의 식별·등록 설정입니다.
     * @return 생성 객체의 소유권입니다. 생성 정책, 팩터리, 클래스 일치, 식별 설정 또는 등록에 실패하면 nullptr입니다.
     */
    std::unique_ptr<UObject> CreateInstance(const FObjectConstructionParameters& Parameters = {}) const;
    /**
     * @brief 필요할 때 클래스 기본 객체를 한 번 생성해 반환합니다.
     * @return 클래스가 소유한 기본 객체입니다. 팩터리 부재, 생성 제한, 재진입 또는 생성 실패이면 nullptr일 수 있습니다.
     * @note 기본 객체 생성은 재귀 뮤텍스로 보호하며 객체 저장소에 등록하지 않습니다.
     */
    const UObject* GetDefaultObject() const;
    /**
     * @brief 쓰기 가능한 프로퍼티를 각 프로퍼티의 기본 객체 값으로 복원합니다.
     * @param Object 현재 클래스와 호환되는 대상 객체입니다.
     * @return 대상 타입과 기본 객체가 유효하고 모든 쓰기가 성공하면 true입니다.
     * @note 중간 실패 시 이미 적용한 변경을 되돌리지 않습니다.
     */
    bool ResetToDefault(UObject& Object) const;
    /**
     * @brief 직접 선언한 함수 메타데이터의 소유권을 인수합니다.
     * @param Function 등록할 함수입니다.
     * @note 빈 함수 또는 현재 클래스 안의 함수 이름 중복이면 Critical 오류를 보고합니다.
     */
    void AddFunction(std::unique_ptr<UFunction> Function);
    /**
     * @brief 현재 클래스부터 기반 클래스 순서로 함수를 찾습니다.
     * @param Name 검색할 함수 이름입니다.
     * @return 가장 가까운 함수 메타데이터입니다. 없으면 nullptr입니다.
     */
    const UFunction* FindFunction(FName Name) const;
    /**
     * @brief 선택적으로 기반 함수를 포함한 함수 목록을 반환합니다.
     * @param IncludeInherited 기반 함수 포함 여부입니다.
     * @return 비소유 함수 포인터 배열입니다. 같은 이름의 파생 함수가 기반 함수를 대체합니다.
     */
    TArray<const UFunction*> GetFunctions(bool IncludeInherited = true) const;

private:
    /**
     * @brief 클래스 생성 정책 비트입니다.
     */
    EClassFlags mClassFlags{};
    /**
     * @brief 객체 생성 팩터리입니다.
     */
    FFactory mFactory{};
    /**
     * @brief 클래스 기본 객체의 지연 생성을 보호하는 뮤텍스입니다.
     */
    mutable std::recursive_mutex mDefaultMutex{};
    /**
     * @brief 클래스 기본 객체 생성의 재진입을 감지하는 상태입니다.
     */
    mutable bool mCreatingDefault{};
    /**
     * @brief 지연 생성해 클래스가 소유하는 기본 객체입니다.
     */
    mutable std::unique_ptr<UObject> mDefaultObject{};
    /**
     * @brief 직접 선언한 리플렉션 함수를 등록 순서로 소유하는 목록입니다.
     */
    TArray<std::unique_ptr<UFunction>> mFunctions{};
};

template <typename T>
bool UClass::IsChildOf() const {
    static_assert(std::is_same_v<typename T::ReflectionOwner, T>);

    return IsChildOf(T::StaticClass());
}
