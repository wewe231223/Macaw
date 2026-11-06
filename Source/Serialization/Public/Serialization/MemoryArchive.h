/**
 * @file MemoryArchive.h
 * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브에 대한 선언입니다.
 * @ingroup Serialization
 */

#pragma once

#include "Core/Archive.h"

namespace Serialization {

    /**
     * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브를 담당합니다.
     */
    class FMemoryArchive final : public Core::FArchive {
    public:
        /**
         * @brief 기반 아카이브의 복합 값 직렬화 오버로드를 함께 노출합니다.
         * @param[in] Name 직렬화할 필드의 이름입니다.
         * @param[in,out] Value 저장 시 읽고 로드 시 복원할 값입니다.
         */
        using Core::FArchive::Serialize;

    public:
        /**
         * @brief 바이트를 추가할 배열로 저장 아카이브를 구성합니다.
         * @param[in,out] Bytes 바이너리 데이터를 읽거나 기록할 바이트 배열입니다.
         */
        explicit FMemoryArchive(Core::TArray<Core::Uint8>& Bytes);

        /**
         * @brief 읽기 전용 바이트 배열로 로드 아카이브를 구성합니다.
         * @param[in] Bytes 바이너리 데이터를 읽거나 기록할 바이트 배열입니다.
         */
        explicit FMemoryArchive(const Core::TArray<Core::Uint8>& Bytes);

    public:
        /**
         * @brief 로드 아카이브에서 아직 읽지 않은 바이트 수를 반환합니다.
         * @return 로드 아카이브에서 아직 읽지 않은 바이트 수를 나타내는 정수 값입니다.
         */
        std::size_t GetRemainingBytes() const;

        /**
         * @brief 현재 컨테이너에 지정한 이름의 필드가 있는지 검사합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool HasField(std::string_view Name) const override;

        /**
         * @brief 이름으로 지정한 객체 컨테이너에 진입합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool BeginObject(std::string_view Name) override;

        /**
         * @brief 현재 객체 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
         */
        void EndObject() override;

        /**
         * @brief 배열 컨테이너에 진입하고 로드 시 원소 개수를 읽습니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Count 저장할 배열 길이이며 로드 시 읽어 온 배열 길이를 받습니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool BeginArray(std::string_view Name, Core::Uint32& Count) override;

        /**
         * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
         */
        void EndArray() override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, bool& Value) override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::Int32& Value) override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::Uint32& Value) override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::Uint64& Value) override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, float& Value) override;

        /**
         * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
         * @param[in] Name 조회하거나 기록할 이름입니다.
         * @param[in] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
         */
        void Serialize(std::string_view Name, Core::FString& Value) override;

    private:
        /**
         * @brief 아카이브 모드에 따라 바이트를 추가하거나 범위를 검사하여 읽습니다.
         * @param[in,out] Value 읽거나 기록·변환할 값입니다.
         * @param[in] Size 처리할 데이터의 바이트 수입니다.
         */
        void SerializeBytes(void* Value, std::size_t Size);

    private:
        /**
         * @brief 저장 바이트를 추가할 배열의 비소유 주소입니다.
         */
        Core::TArray<Core::Uint8>* mWriteBytes{};

        /**
         * @brief 로드할 바이트 배열의 비소유 주소입니다.
         */
        const Core::TArray<Core::Uint8>* mReadBytes{};

        /**
         * @brief 로드 바이트 배열에서 다음에 읽을 위치입니다.
         */
        std::size_t mReadOffset{};
    };

}
