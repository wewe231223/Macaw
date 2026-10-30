/**
 * @file UScriptStruct.h
 * @brief 일반 C++ 타입의 프로퍼티와 구조체 상속 관계를 기술합니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/UStruct.h"

namespace CoreUObject {

    /**
     * @brief 일반 C++ 타입의 프로퍼티와 구조체 상속 관계를 기술합니다.
     * @ingroup CoreUObject
     */
    class UScriptStruct final : public CoreUObject::UStruct {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(UScriptStruct, CoreUObject::UStruct);

    public:
        /**
         * @brief UScriptStruct의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Size 네이티브 타입의 sizeof 바이트 수입니다.
         * @param[in] Parent 부모 타입의 메타데이터이며, 없으면 nullptr입니다.
         */
        UScriptStruct(Core::FString Name, std::size_t Size, const CoreUObject::UScriptStruct* Parent = nullptr);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UScriptStruct() override;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(UScriptStruct);

        /**
         * @brief 부모 구조체 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::UScriptStruct* GetSuperStruct() const;

        /**
         * @brief 자기 자신을 포함한 상속 체인에 지정한 부모 타입이 있는지 검사합니다.
         * @param[in] Parent 부모 타입의 메타데이터이며, 없으면 nullptr입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsChildOf(const CoreUObject::UScriptStruct* Parent) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 부모 구조체 메타데이터이며, 없으면 nullptr입니다.
         */
        const CoreUObject::UScriptStruct* mParent{};
        ReflectProperty(mParent, "Parent", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
