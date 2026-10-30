/**
 * @file UClass.h
 * @brief UObject 상속 관계, 팩터리와 상속된 프로퍼티를 기술하는 메타데이터입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"
#include "CoreUObject/Reflection/EClassFlags.h"

namespace CoreUObject {

    class UObject;

    /**
     * @brief UObject 상속 관계, 팩터리와 상속된 프로퍼티를 기술하는 메타데이터입니다.
     * @ingroup CoreUObject
     */
    class UClass final : public CoreUObject::UStruct {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UClass, CoreUObject::UStruct);

    public:
        /**
         * @brief 소유권을 가진 UObject를 반환하는 선택적인 네이티브 팩터리입니다.
         */
        using FFactory = std::unique_ptr<CoreUObject::UObject> (*)();

    public:
        /**
         * @brief UClass의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Size 뷰포트 크기 또는 데이터의 바이트 수입니다.
         * @param[in] Parent 부모 메타데이터 또는 부모를 조회하는 콜백입니다.
         * @param[in] Factory 선택적인 네이티브 인스턴스 생성 함수입니다.
         * @param[in] Flags 타입 또는 프로퍼티의 정책 비트 플래그입니다.
         */
        UClass(Core::FString Name, std::size_t Size, const CoreUObject::UClass* Parent, FFactory Factory, CoreUObject::EClassFlags Flags = CoreUObject::EClassFlags::None);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UClass() override;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(UClass);

        /**
         * @brief 부모 클래스 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::UClass* GetSuperClass() const;

        /**
         * @brief 자기 자신을 포함한 상속 체인에 지정한 부모 타입이 있는지 검사합니다.
         * @param[in] Parent 부모 메타데이터 또는 부모를 조회하는 콜백입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsChildOf(const CoreUObject::UClass* Parent) const;

        /**
         * @brief 요청한 클래스 플래그 중 하나라도 설정되어 있는지 검사합니다.
         * @param[in] Flags 타입 또는 프로퍼티의 정책 비트 플래그입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool HasAnyFlags(CoreUObject::EClassFlags Flags) const;

        /**
         * @brief 클래스 정책과 팩터리가 인스턴스 생성을 허용하는지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool CanCreate() const;

        /**
         * @brief 생성 가능한 클래스의 등록된 팩터리를 호출합니다.
         * @return 생성한 객체의 단독 소유권이며, 생성 또는 검증 실패 시 nullptr입니다.
         */
        std::unique_ptr<CoreUObject::UObject> CreateInstance() const;

        /**
         * @brief 프로퍼티 등록을 마무리하고 조회용 목록을 확정합니다.
         */
        void Seal() override;

        /**
         * @brief 상속된 항목을 포함한 프로퍼티 주소 목록을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<const CoreUObject::FProperty*>& GetAllProperties() const;

        /**
         * @brief 상속된 프로퍼티를 포함하여 이름으로 프로퍼티를 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::FProperty* FindClassProperty(const Core::FString& Name) const;

        /**
         * @brief 상속된 프로퍼티를 포함하여 클래스 값을 직렬화합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
         */
        /**
         * @param[in] Excluded 직렬화에서 제외할 프로퍼티이며 nullptr이면 제외하지 않습니다.
         */
        void SerializeClassProperties(void* Container, Core::FArchive& Archive, const CoreUObject::FProperty* Excluded = nullptr) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 부모 타입 또는 부모 조회 콜백입니다.
         */
        const CoreUObject::UClass* mParent{};
        ReflectProperty(mParent, "Parent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 객체 생성 팩터리입니다.
         */
        FFactory mFactory{};
        ReflectProperty(mFactory, "Factory", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 동작 정책 비트 플래그입니다.
         */
        CoreUObject::EClassFlags mFlags{};
        ReflectProperty(mFlags, "Flags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 상속된 항목을 포함한 프로퍼티 주소 목록입니다.
         */
        Core::TArray<const CoreUObject::FProperty*> mAllProperties{};
        ReflectProperty(mAllProperties, "AllProperties", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 상속 프로퍼티 목록이 확정되었는지 여부입니다.
         */
        bool mClassSealed{};
        ReflectProperty(mClassSealed, "ClassSealed", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
