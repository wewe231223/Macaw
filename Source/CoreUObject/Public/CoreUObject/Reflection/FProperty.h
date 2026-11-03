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

#include <string_view>

namespace Core {
    class FArchive;
}

namespace CoreUObject {

    class UStruct;
    struct FNativeType;
    struct FArrayOps;

    /**
     * @brief 타입이 지워진 프로퍼티의 읽기, 쓰기, 편집 및 직렬화 규약입니다.
     * @ingroup CoreUObject
     */
    class FProperty {
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
         * @param[in] Type 프로퍼티 값의 저장 형식입니다.
         * @param[in] Flags 프로퍼티의 편집·직렬화 정책 플래그입니다.
         * @param[in] Reader 컨테이너에서 값을 읽는 콜백입니다.
         * @param[in] Writer 컨테이너에 값을 쓰는 콜백입니다.
         * @param[in] Struct 복합 프로퍼티의 선택적인 구조체 메타데이터입니다.
         * @param[in] Values 등록할 열거값 목록입니다.
         * @param[in] Loader 읽기 전용 프로퍼티의 로드에 사용할 선택적인 콜백입니다.
         * @param[in] NativeType 네이티브 타입 식별 메타데이터입니다.
         * @param[in] ArrayOps 배열 프로퍼티의 함수 테이블이며 배열이 아니면 nullptr입니다.
         */
        FProperty(Core::FString Name, CoreUObject::EPropertyType Type, Core::EPropertyFlags Flags, FReader Reader, FWriter Writer, const CoreUObject::UStruct* Struct = nullptr, Core::TArray<CoreUObject::FEnumValue> Values = {}, FWriter Loader = nullptr, const CoreUObject::FNativeType* NativeType = nullptr, const CoreUObject::FArrayOps* ArrayOps = nullptr);

    public:
        /**
         * @brief 프로퍼티 이름을 반환합니다.
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
         * @param[in] Flag 검사할 플래그 비트입니다.
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
         * @brief 배열 프로퍼티의 함수 테이블을 반환합니다.
         * @return 조회한 값의 비소유 주소이며, 배열이 아니면 nullptr입니다.
         */
        const CoreUObject::FArrayOps* GetArrayOps() const;

        /**
         * @brief 컨테이너에 등록된 읽기 함수를 호출하여 프로퍼티 값을 가져옵니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @return 읽은 프로퍼티 값입니다.
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
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void SerializeValue(void* Container, Core::FArchive& Archive) const;

        /**
         * @brief 이 프로퍼티와 하위 원소의 객체 참조에 핸들을 연결합니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in] Objects 이 장면에서 읽은 객체의 대응표입니다.
         */
        void BindObjectReferences(void* Container, const CoreUObject::FObjectGuidMap& Objects) const;

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

        /**
         * @brief 저장 정책 검사를 마친 값을 지정한 이름으로 직렬화합니다.
         * @param[in] Name 아카이브에 기록할 이름이며 배열 원소는 인덱스입니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void SerializeNamed(std::string_view Name, void* Container, Core::FArchive& Archive) const;

        /**
         * @brief 배열을 원소 단위로 직렬화합니다. 로드는 작업용 배열을 채운 뒤 한 번에 커밋합니다.
         * @param[in] Name 아카이브에 기록할 이름입니다.
         * @param[in] Container 프로퍼티 소유 타입의 유효한 인스턴스 주소입니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         */
        void SerializeArray(std::string_view Name, void* Container, Core::FArchive& Archive) const;

    private:
        /**
         * @brief 프로퍼티 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 프로퍼티 저장 타입입니다.
         */
        CoreUObject::EPropertyType mType{};

        /**
         * @brief 동작 정책 비트 플래그입니다.
         */
        Core::EPropertyFlags mFlags{};

        /**
         * @brief 프로퍼티 읽기 콜백입니다.
         */
        FReader mReader{};

        /**
         * @brief 프로퍼티 편집 쓰기 콜백입니다.
         */
        FWriter mWriter{};

        /**
         * @brief 읽기 전용 프로퍼티의 로드용 쓰기 콜백입니다.
         */
        FWriter mLoader{};

        /**
         * @brief 복합 값의 구조체 메타데이터입니다.
         */
        const CoreUObject::UStruct* mStruct{};

        /**
         * @brief 열거값 이름과 정수 값 목록입니다.
         */
        Core::TArray<CoreUObject::FEnumValue> mEnumValues{};

        /**
         * @brief 네이티브 타입 식별 메타데이터입니다.
         */
        const CoreUObject::FNativeType* mNativeType{};

        /**
         * @brief 배열 프로퍼티의 함수 테이블입니다.
         */
        const CoreUObject::FArrayOps* mArrayOps{};
    };

}
