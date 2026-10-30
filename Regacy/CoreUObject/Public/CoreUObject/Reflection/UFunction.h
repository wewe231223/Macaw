/**
 * @file UFunction.h
 * @brief 객체, 인수와 반환 값 타입을 검증하는 리플렉션 함수 메타데이터를 제공합니다.
 */

#pragma once

#include "CoreUObject/Reflection/ReflectionMacros.h"

#include "CoreUObject/Reflection/UStruct.h"
#include <span>

/**
 * @brief 소유 클래스, 인수 타입 목록과 호출 함수를 보관하는 리플렉션 함수입니다.
 */
class UFunction : public UStruct {
public:
    /**
     * @brief 자체 리플렉션 선언의 소유 타입입니다.
     */
    using ReflectionOwner = UFunction;

    /**
     * @brief 함수 필드와 인수 프로퍼티를 관리하는 기반 타입입니다.
     */
    using Super = UStruct;

    /**
     * @brief 객체와 타입 소거 인수를 받아 결과 값과 성공 여부를 반환하는 호출 함수 타입입니다.
     */
    using FInvoker = bool (*)(UObject&, std::span<const FPropertyValue>, FPropertyValue&);

public:
    /**
     * @brief 함수 이름, 소유 클래스와 호출 함수를 설정합니다.
     * @param Name 리플렉션 함수 이름입니다.
     * @param Owner 함수의 소유 클래스이자 Outer입니다.
     * @param Invoker 실제 네이티브 함수를 호출할 어댑터입니다.
     */
    UFunction(FName Name, UClass* Owner, FInvoker Invoker);

    /**
     * @brief 함수와 인수 프로퍼티 메타데이터를 해제합니다.
     */
    ~UFunction() override = default;

public:
    /**
     * @brief UFunction 클래스의 리플렉션 메타데이터를 반환합니다.
     * @return UFunction의 클래스 정보입니다.
     */
    ReflectClass(UFunction, EClassFlags::Metadata | EClassFlags::NoFactory);

    /**
     * @brief 인수 프로퍼티와 타입을 선언 순서로 추가합니다.
     * @param Name 인수 이름입니다.
     * @param Type 인수의 정확한 네이티브 타입 정보입니다.
     */
    void AddParameter(FName Name, const FNativeType* Type);

    /**
     * @brief 반환 타입을 설정하고 값 반환일 때 ReturnValue 프로퍼티를 추가합니다.
     * @param Type 반환 타입입니다. nullptr이면 void 반환입니다.
     * @note 같은 이름의 프로퍼티를 다시 추가하지 않도록 값 반환 타입 설정은 등록 과정에서 한 번 수행합니다.
     */
    void SetReturnType(const FNativeType* Type);

    /**
     * @brief 등록한 반환 타입을 반환합니다.
     * @return 반환 타입 정보입니다. void 반환이면 nullptr입니다.
     */
    const FNativeType* GetReturnType() const;

    /**
     * @brief 객체 상속 관계와 인수 개수·정확한 타입을 확인한 뒤 함수를 호출합니다.
     * @param Object 소유 클래스와 호환되는 호출 대상입니다.
     * @param Arguments 선언 순서의 인수 값입니다.
     * @param[out] Result 호출 반환 값입니다. 호출 시작 시 비우며 실패 시에도 비웁니다.
     * @return 검증, 호출과 반환 타입 일치가 모두 성공하면 true입니다.
     * @note 숫자 타입 변환이나 기본 인수 적용은 수행하지 않습니다.
     */
    bool Invoke(UObject& Object, std::span<const FPropertyValue> Arguments, FPropertyValue& Result) const;

private:
    /**
     * @brief 호출 대상의 상속 관계를 검사할 소유 클래스입니다.
     */
    UClass* mOwnerClass{};

    /**
     * @brief 네이티브 함수를 호출하는 어댑터입니다.
     */
    FInvoker mInvoker{};

    /**
     * @brief 선언 순서의 정확한 인수 타입 정보입니다.
     */
    TArray<const FNativeType*> mParameterTypes{};

    /**
     * @brief 반환 타입 정보입니다. nullptr이면 void 반환입니다.
     */
    const FNativeType* mReturnType{};
};
