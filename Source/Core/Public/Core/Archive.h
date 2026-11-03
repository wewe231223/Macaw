/**
 * @file Archive.h
 * @brief 로드·저장 공통 아카이브와 복합 수학 값 직렬화를 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Guid.h"
#include "Core/GuidHash.h"
#include "Core/Math.h"

#include <string_view>

namespace Core {

    /**
     * @brief 아카이브가 값을 읽을지 쓸지를 지정합니다.
     * @ingroup Core
     */
    enum class EArchiveMode {
        /**
         * @brief 저장된 값에서 메모리로 읽습니다.
         */
        Loading,
        /**
         * @brief 메모리 값을 아카이브에 씁니다.
         */
        Saving
    };

    /**
     * @brief 로드와 저장에 공통으로 사용하는 이름 기반 직렬화 인터페이스입니다.
     * @ingroup Core
     */
    class FArchive {
    public:
        /**
         * @brief FArchive의 초기 상태와 전달받은 의존성을 구성합니다.
         * @param[in] Mode 로드 또는 저장 모드입니다.
         */
        explicit FArchive(Core::EArchiveMode Mode);

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        virtual ~FArchive();

    public:
        /**
         * @brief 아카이브가 로드 모드인지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsLoading() const;

        /**
         * @brief 객체 GUID를 재매핑하는 복제용 아카이브인지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsDuplicating() const;

        /**
         * @brief 객체 복제에 사용할 GUID 대응표의 비소유 주소를 설정합니다.
         * @param[in] Remapping 원본 GUID와 복제 GUID 사이의 대응표입니다.
         */
        void SetObjectGuidRemapping(const Core::TMap<Core::FGuid, Core::FGuid, Core::FGuidHash>* Remapping);

        /**
         * @brief 객체 GUID를 직렬화하고 로드 시 복제용 GUID 재매핑을 적용합니다.
         * @param[in] Name 직렬화할 필드의 이름입니다.
         * @param[in,out] Value 읽거나 기록·변환할 값입니다.
         */
        void SerializeObjectGuid(std::string_view Name, Core::FGuid& Value);

        /**
         * @brief 등록된 GUID 대응표로 객체 참조를 복제 대상의 GUID로 변환합니다.
         * @param[in,out] Value 읽거나 기록·변환할 값입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool RemapObjectReference(Core::FGuid& Value);

        /**
         * @brief 복제 범위 안의 객체 참조만 복제 대상의 GUID로 변환합니다.
         * @param[in,out] Value 변환할 참조이며 복제 범위 밖이면 그대로 유지합니다.
         * @note 사라졌거나 함께 복제되지 않은 대상을 허용하는 약한 참조에 사용합니다.
         */
        void RemapWeakObjectReference(Core::FGuid& Value);

        /**
         * @brief 아카이브에 오류 메시지가 기록되어 있는지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool HasError() const;

        /**
         * @brief 처음 발생한 오류 메시지를 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FString& GetError() const;

        /**
         * @brief 아카이브에 처음 발생한 오류 메시지를 보관합니다.
         * @param[in] Message 기록할 오류 메시지입니다.
         */
        void SetError(const Core::FString& Message);

        /**
         * @brief 현재 컨테이너에 지정한 이름의 필드가 있는지 검사합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool HasField(std::string_view Name) const = 0;

        /**
         * @brief 이름으로 지정한 객체 컨테이너에 진입합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool BeginObject(std::string_view Name) = 0;

        /**
         * @brief 현재 객체 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
         */
        virtual void EndObject() = 0;

        /**
         * @brief 배열 컨테이너에 진입하고 로드 시 원소 개수를 읽습니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Count 저장할 배열 길이이며 로드 시 읽어 온 배열 길이를 받습니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        virtual bool BeginArray(std::string_view Name, Core::Uint32& Count) = 0;

        /**
         * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
         */
        virtual void EndArray() = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, bool& Value) = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, Core::Int32& Value) = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, Core::Uint32& Value) = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, Core::Uint64& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, float& Value) = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        virtual void Serialize(std::string_view Name, Core::FString& Value) = 0;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FGuid& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FVector2& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FVector& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FVector4& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FQuat& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FRotator& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FMatrix& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FTransform& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FBox& Value);

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FRay& Value);

    private:
        /**
         * @brief 현재 동작 모드입니다.
         */
        Core::EArchiveMode mMode{};

        /**
         * @brief 복제 시 원본 GUID를 새 GUID로 변환하는 비소유 대응표입니다.
         */
        const Core::TMap<Core::FGuid, Core::FGuid, Core::FGuidHash>* mObjectGuidRemapping{};

        /**
         * @brief 처음 발생한 오류 메시지입니다.
         */
        Core::FString mError{};
    };

}
