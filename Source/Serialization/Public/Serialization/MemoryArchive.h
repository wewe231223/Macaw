/**
 * @file MemoryArchive.h
 * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브에 대한 선언입니다.
 * @ingroup Serialization
 */

#pragma once

#include "Core/Archive.h"

/**
 * @brief 바이트 배열을 순차적으로 읽고 쓰는 바이너리 아카이브를 담당합니다.
 */
class FMemoryArchive final : public FArchive {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(FMemoryArchive, FArchive);
    /**
     * @brief 기반 아카이브의 복합 값 직렬화 오버로드를 함께 노출합니다.
     * @param[in] Name 직렬화할 필드의 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 복원할 값입니다.
     */
    using FArchive::Serialize;

public:
    /**
     * @brief 바이트를 추가할 배열로 저장 아카이브를 구성합니다.
     * @param[in,out] Bytes 바이너리 데이터를 읽거나 기록할 바이트 배열입니다.
     */
    explicit FMemoryArchive(TArray<Uint8>& Bytes);
    /**
     * @brief 읽기 전용 바이트 배열로 로드 아카이브를 구성합니다.
     * @param[in] Bytes 바이너리 데이터를 읽거나 기록할 바이트 배열입니다.
     */
    explicit FMemoryArchive(const TArray<Uint8>& Bytes);

public:
    /**
     * @brief 이 타입의 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FMemoryArchive);
    /**
     * @brief 로드 아카이브에서 아직 읽지 않은 바이트 수를 반환합니다.
     * @return 로드 아카이브에서 아직 읽지 않은 바이트 수를 나타내는 정수 값입니다.
     */
    std::size_t GetRemainingBytes() const;
    bool HasField(std::string_view Name) const override;
    bool BeginObject(std::string_view Name) override;
    void EndObject() override;
    bool BeginArray(std::string_view Name, Uint32& Count) override;
    void EndArray() override;
    void Serialize(std::string_view Name, bool& Value) override;
    void Serialize(std::string_view Name, Int32& Value) override;
    void Serialize(std::string_view Name, Uint32& Value) override;
    void Serialize(std::string_view Name, Uint64& Value) override;
    void Serialize(std::string_view Name, float& Value) override;
    void Serialize(std::string_view Name, FString& Value) override;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();
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
    TArray<Uint8>* mWriteBytes{};
    ReflectProperty(mWriteBytes, "WriteBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 로드할 바이트 배열의 비소유 주소입니다.
     */
    const TArray<Uint8>* mReadBytes{};
    ReflectProperty(mReadBytes, "ReadBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 로드 바이트 배열에서 다음에 읽을 위치입니다.
     */
    std::size_t mReadOffset{};
    ReflectProperty(mReadOffset, "ReadOffset", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
