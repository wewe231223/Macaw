/**
 * @file FReflectionRegistry.h
 * @brief 네이티브 타입 정의를 검사하고 프로세스 수명의 메타데이터를 등록합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/UScriptStruct.h"
#include "CoreUObject/Reflection/UEnum.h"
#include "CoreUObject/Reflection/FNativeType.h"

namespace CoreUObject {

    /**
     * @brief 네이티브 타입 정의를 검사하고 프로세스 수명의 메타데이터를 등록합니다.
     * @ingroup CoreUObject
     */
    class FReflectionRegistry {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FReflectionRegistry);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FReflectionRegistry);

        /**
         * @brief 소유권을 가진 UObject를 반환하는 선택적인 네이티브 팩터리입니다.
         */
        using FFactory = CoreUObject::UClass::FFactory;

        /**
         * @brief UClass의 멤버 메타데이터를 채우는 함수 포인터입니다.
         */
        using FClassBuilder = void (*)(CoreUObject::UClass*);

        /**
         * @brief UScriptStruct의 멤버 메타데이터를 채우는 함수 포인터입니다.
         */
        using FStructBuilder = void (*)(CoreUObject::UScriptStruct*);

        /**
         * @brief UEnum의 열거값을 채우는 함수 포인터입니다.
         */
        using FEnumBuilder = void (*)(CoreUObject::UEnum*);

    public:
        /**
         * @brief 같은 이름의 정의와 호환성을 검사한 뒤 클래스 메타데이터를 등록합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Parent 부모 클래스 메타데이터를 반환하는 함수이며, 루트이면 nullptr입니다.
         * @param[in] NativeType 네이티브 타입 식별 메타데이터입니다.
         * @param[in] Flags 클래스 생성 정책 플래그입니다.
         * @param[in] Factory 선택적인 네이티브 인스턴스 생성 함수입니다.
         * @param[in] Builder 타입의 프로퍼티나 열거값을 채우는 함수입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UClass* DefineClass(Core::FString Name, CoreUObject::UClass* (*Parent)(), const CoreUObject::FNativeType* NativeType, CoreUObject::EClassFlags Flags, FFactory Factory, FClassBuilder Builder);

        /**
         * @brief 같은 이름의 정의와 호환성을 검사한 뒤 구조체 메타데이터를 등록합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] NativeType 네이티브 타입 식별 메타데이터입니다.
         * @param[in] Builder 타입의 프로퍼티나 열거값을 채우는 함수입니다.
         * @param[in] Parent 부모 타입의 메타데이터이며, 없으면 nullptr입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UScriptStruct* DefineStruct(Core::FString Name, const CoreUObject::FNativeType* NativeType, FStructBuilder Builder, const CoreUObject::UScriptStruct* Parent = nullptr);

        /**
         * @brief 같은 이름의 정의와 호환성을 검사한 뒤 열거형 메타데이터를 등록합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] NativeType 네이티브 타입 식별 메타데이터입니다.
         * @param[in] Builder 타입의 프로퍼티나 열거값을 채우는 함수입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UEnum* DefineEnum(Core::FString Name, const CoreUObject::FNativeType* NativeType, FEnumBuilder Builder);

        /**
         * @brief 열거형 빌더에서 전달한 열거값 목록을 등록합니다.
         * @param[in,out] Information 열거값을 추가할 열거형 메타데이터입니다.
         * @param[in] Values 등록할 열거값 목록입니다.
         */
        static void AddEnumValues(CoreUObject::UEnum* Information, std::initializer_list<CoreUObject::FEnumValue> Values);

        /**
         * @brief 등록된 클래스 메타데이터를 이름으로 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UClass* FindClass(const Core::FString& Name);

        /**
         * @brief 지정한 부모 클래스를 상속한 등록 클래스를 수집합니다.
         * @param[in] Parent 파생 클래스를 찾을 부모 클래스입니다.
         * @return 상속한 클래스 메타데이터의 비소유 주소 목록입니다.
         */
        static Core::TArray<const CoreUObject::UClass*> GetDerivedClasses(const CoreUObject::UClass* Parent);

        /**
         * @brief 등록된 구조체 메타데이터를 이름으로 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UScriptStruct* FindStruct(const Core::FString& Name);

        /**
         * @brief 등록된 열거형 메타데이터를 이름으로 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        static CoreUObject::UEnum* FindEnum(const Core::FString& Name);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();
    };

}
