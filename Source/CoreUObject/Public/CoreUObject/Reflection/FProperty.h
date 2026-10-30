/**
 * @file FProperty.h
 * @brief 타입이 지워진 프로퍼티의 읽기, 쓰기, 편집 및 직렬화 규약입니다.
 * @ingroup CoreUObject
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "CoreUObject/Reflection/EPropertyType.h"
#include "Core/Reflection/EPropertyFlags.h"
#include "CoreUObject/Reflection/FEnumValue.h"

namespace Core {
    class FArchive;
}

namespace CoreUObject {

    class UStruct;
    struct FNativeType;

    /**
     * @brief 타입이 지워진 프로퍼티의 읽기, 쓰기, 편집 및 직렬화 규약입니다.
     * @ingroup CoreUObject
     */
    class FProperty {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FProperty);

    public:
        /**
         * @brief 컨테이너에서 프로퍼티 variant를 읽는 함수 포인터입니다.
         */
        using FReader = CoreUObject::FPropertyValue (*)(const void*);

        /**
         * @brief 컨테이너에 프로퍼티 variant를 쓰고 성공 여부를 반환하는 함수 포인터입니다.
         */
        using FWriter = bool (*)(void*, const CoreUObject::FPropertyValue&);

    public:
        /**
         * @brief FProperty의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Type 생성, 조회 또는 표시할 타입입니다.
         * @param[in] Flags 타입 또는 프로퍼티의 정책 비트 플래그입니다.
         * @param[in] Reader 컨테이너에서 값을 읽는 콜백입니다.
         * @param[in] Writer 컨테이너에 값을 쓰는 콜백입니다.
         * @param[in] Struct 복합 프로퍼티의 선택적인 구조체 메타데이터입니다.
         * @param[in] Values 등록할 열거값 목록입니다.
         * @param[in] Loader 읽기 전용 프로퍼티의 로드에 사용할 선택적인 콜백입니다.
         * @param[in] NativeType 네이티브 타입 식별 메타데이터입니다.
         */
        FProperty(Core::FString Name, CoreUObject::EPropertyType Type, Core::EPropertyFlags Flags, FReader Reader, FWriter Writer, const CoreUObject::UStruct* Struct = nullptr, Core::TArray<CoreUObject::FEnumValue> Values = {}, FWriter Loader = nullptr, const CoreUObject::FNativeType* NativeType = nullptr);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FProperty);

        /**
         * @brief 이름 또는 표시 문자열을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FString& GetName() const;

        /**
         * @brief 프로퍼티 저장 타입을 반환합니다.
         * @return 프로퍼티 저장 타입입니다.
         */
        CoreUObject::EPropertyType GetType() const;

        /**
         * @brief 프로퍼티 값의 네이티브 타입 식별 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::FNativeType* GetNativeType() const;

        /**
         * @brief 요청한 프로퍼티 플래그가 설정되어 있는지 검사합니다.
         * @param[in] Flag Flag에 적용할 입력 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool HasFlag(Core::EPropertyFlags Flag) const;

        /**
         * @brief 복합 값의 구조체 메타데이터를 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
         */
        const CoreUObject::UStruct* GetStruct() const;

        /**
         * @brief 열거값 이름과 정수 값 목록을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<CoreUObject::FEnumValue>& GetEnumValues() const;

        /**
         * @brief 컨테이너에 등록된 읽기 함수를 호출하여 프로퍼티 값을 가져옵니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        CoreUObject::FPropertyValue ReadValue(const void* Container) const;

        /**
         * @brief 값 타입과 쓰기 정책을 검사한 뒤 등록된 함수로 프로퍼티를 변경합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in] Value 프로퍼티 타입에 대응하는 variant 값입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         * @note 일반 편집 경로는 ReadOnly 프로퍼티를 거부합니다. 로드 전용 콜백은 SerializeValue()에서 선택합니다.
         */
        bool WriteValue(void* Container, const CoreUObject::FPropertyValue& Value) const;

        /**
         * @brief 로드 전용 쓰기 경로로 동일 프로퍼티의 값을 복사합니다.
         * @param[in,out] Destination 이 프로퍼티를 포함한 대상 컨테이너입니다.
         * @param[in] Source 같은 레이아웃을 가진 원본 컨테이너입니다.
         * @return 읽고 로드할 수 있는 값의 복사에 성공하면 true입니다.
         * @note Serialize 플래그와 일반 편집의 ReadOnly 제한을 적용하지 않습니다. Transient, 불투명 주소 값과 nullptr
         * 컨테이너는 거부합니다.
         */
        bool CopyValue(void* Destination, const void* Source) const;

        /**
         * @brief 프로퍼티의 저장 정책과 타입에 맞추어 값을 직렬화합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in,out] Archive 로드 또는 저장 모드를 가진 아카이브입니다.
         */
        void SerializeValue(void* Container, Core::FArchive& Archive) const;

    private:
        /**
         * @brief 값 타입과 쓰기 정책을 검사한 뒤 등록된 함수로 프로퍼티를 변경합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in] Value 프로퍼티 타입에 대응하는 variant 값입니다.
         * @param[in] Writer 컨테이너에 값을 쓰는 콜백입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         * @note 일반 편집 경로는 ReadOnly 프로퍼티를 거부합니다. 로드 전용 콜백은 SerializeValue()에서 선택합니다.
         */
        bool WriteValue(void* Container, const CoreUObject::FPropertyValue& Value, FWriter Writer) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 이름 또는 표시 문자열입니다.
         */
        Core::FString mName{};
        ReflectProperty(mName, "Name", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로퍼티 저장 타입입니다.
         */
        CoreUObject::EPropertyType mType{};
        ReflectProperty(mType, "Type", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 동작 정책 비트 플래그입니다.
         */
        Core::EPropertyFlags mFlags{};
        ReflectProperty(mFlags, "Flags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 프로퍼티 읽기 콜백입니다.
         */
        FReader mReader{};
        ReflectProperty(mReader, "Reader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 프로퍼티 편집 쓰기 콜백입니다.
         */
        FWriter mWriter{};
        ReflectProperty(mWriter, "Writer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 읽기 전용 프로퍼티의 로드용 쓰기 콜백입니다.
         */
        FWriter mLoader{};
        ReflectProperty(mLoader, "Loader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 복합 값의 구조체 메타데이터입니다.
         */
        const CoreUObject::UStruct* mStruct{};
        ReflectProperty(mStruct, "Struct", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 열거값 이름과 정수 값 목록입니다.
         */
        Core::TArray<CoreUObject::FEnumValue> mEnumValues{};
        ReflectProperty(mEnumValues, "EnumValues", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 네이티브 타입 식별 메타데이터입니다.
         */
        const CoreUObject::FNativeType* mNativeType{};
        ReflectProperty(mNativeType, "NativeType", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
