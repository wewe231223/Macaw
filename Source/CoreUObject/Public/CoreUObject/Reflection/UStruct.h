/**
 * @file UStruct.h
 * @brief 네이티브 타입의 이름, 크기와 프로퍼티 메타데이터를 관리합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "CoreUObject/FObjectReference.h"
#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FProperty.h"

namespace CoreUObject {

    /**
     * @brief 네이티브 타입의 이름, 크기와 프로퍼티 메타데이터를 관리합니다.
     * @ingroup CoreUObject
     */
    class UStruct {
    public:
        /**
         * @brief UStruct의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Size 네이티브 타입의 바이트 크기입니다.
         */
        UStruct(Core::FString Name, std::size_t Size);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~UStruct();

    public:
        /**
         * @brief 타입 이름을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 크기를 반환합니다.
         * @return 크기입니다.
         */
        std::size_t GetSize() const;

        /**
         * @brief 타입 메타데이터에 프로퍼티를 추가합니다.
         * @param[in] Property 등록할 프로퍼티입니다.
         */
        void AddProperty(CoreUObject::FProperty Property);

        /**
         * @brief 프로퍼티 등록을 마무리하고 조회용 목록을 확정합니다.
         */
        virtual void Seal();

        /**
         * @brief 조회에 사용하는 프로퍼티 비소유 주소 목록을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<const CoreUObject::FProperty*>& GetProperties() const;

        /**
         * @brief 타입에 선언된 프로퍼티를 이름으로 조회합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::FProperty* FindProperty(const Core::FString& Name) const;

        /**
         * @brief 지정한 프로퍼티가 타입의 프로퍼티 목록에 속하는지 검사합니다.
         * @param[in] Property 소속을 확인할 프로퍼티입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool ContainsProperty(const CoreUObject::FProperty* Property) const;

        /**
         * @brief 저장 가능한 선언 프로퍼티를 아카이브에 순서대로 전달합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void SerializeProperties(void* Container, Core::FArchive& Archive) const;

        /**
         * @brief 선언 프로퍼티에 포함된 객체 참조에 핸들을 연결합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in] Objects 이 장면에서 읽은 객체의 대응표입니다.
         */
        void BindObjectReferences(void* Container, const CoreUObject::FObjectGuidMap& Objects) const;

    private:
        /**
         * @brief 타입 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 네이티브 타입의 sizeof 바이트 수입니다.
         */
        std::size_t mSize{};

        /**
         * @brief 현재 타입이 소유한 프로퍼티 목록입니다.
         */
        Core::TArray<CoreUObject::FProperty> mDeclaredProperties{};

        /**
         * @brief 조회에 사용하는 프로퍼티 비소유 주소 목록입니다.
         */
        Core::TArray<const CoreUObject::FProperty*> mProperties{};

        /**
         * @brief 프로퍼티 등록을 마쳤는지 여부입니다.
         */
        bool mSealed{};
    };

}
