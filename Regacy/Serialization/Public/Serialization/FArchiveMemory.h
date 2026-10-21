/**
 * @file FArchiveMemory.h
 * @brief 바이트 배열을 사용하는 순차 이진 저장·로드 아카이브를 제공합니다.
 */
#pragma once
#include "Core/Archive/FArchive.h"

/**
 * @brief 외부 바이트 배열에 네이티브 값의 이진 표현을 순서대로 저장하거나 읽는 아카이브입니다.
 * @note 저장은 기존 배열 끝에 추가하고 로드는 시작부터 읽습니다. 필드 이름과 객체 스코프는 기록하지 않으며 네이티브 바이트 순서와 타입 레이아웃에 의존합니다.
 * @note FName과 템플릿 값·배열을 처리하는 FArchive의 Serialize 오버로드도 함께 공개합니다.
 * @pre 참조한 배열은 아카이브 사용 중 살아 있어야 하고 로드 중 외부에서 크기를 변경하지 않아야 합니다.
 */
class FArchiveMemory : public FArchive {
public:
    using FArchive::Serialize;

public:
    // Save
    /**
     * @brief 가변 바이트 배열에 추가 기록하는 저장 아카이브를 생성합니다.
     * @param InBytes 저장 결과를 받을 외부 배열입니다. 기존 내용은 지우지 않습니다.
     */
    FArchiveMemory(TArray<Uint8>& InBytes);
    // Load
    /**
     * @brief 상수 바이트 배열을 처음부터 읽는 로드 아카이브를 생성합니다.
     * @param InBytes 읽을 외부 바이트 배열입니다.
     */
    FArchiveMemory(const TArray<Uint8>& InBytes);

public:
    /**
     * @brief 로드 버퍼에서 아직 읽지 않은 바이트 수를 반환합니다.
     * @return 로드 모드의 남은 바이트 수입니다. 저장 모드에서는 0입니다.
     */
    std::size_t GetRemainingBytes() const;

    // Primitives
    /**
     * @brief 불리언을 값이 0 또는 1인 한 바이트로 직렬화합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드한 바이트가 1보다 크면 오류를 설정하고 기존 값을 유지합니다.
     */
    virtual void Serialize(std::string_view Name, bool& Value) override;
    /**
     * @brief Uint8 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Uint8) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Uint8& Value) override;
    /**
     * @brief Int32 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Int32) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Int32& Value) override;
    /**
     * @brief Uint32 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Uint32) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Uint32& Value) override;
    /**
     * @brief Int64 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Int64) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Int64& Value) override;
    /**
     * @brief Uint64 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Uint64) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Uint64& Value) override;
    /**
     * @brief Float32 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Float32) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Float32& Value) override;
    /**
     * @brief Float64 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(Float64) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, Float64& Value) override;

    // Engine Core
    /**
     * @brief 문자열의 Uint32 바이트 길이와 null 종료자를 제외한 내용을 직렬화합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 길이가 Uint32 범위를 넘거나 입력 바이트가 부족하면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, FString& Value) override;
    /**
     * @brief FGuid 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FGuid) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FGuid& Value) override;

    // Math Type
    /**
     * @brief FVector2D 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FVector2D) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FVector2D& Value) override;
    /**
     * @brief FVector3 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FVector3) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FVector3& Value) override;
    /**
     * @brief FVector4 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FVector4) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FVector4& Value) override;
    /**
     * @brief FQuat 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FQuat) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FQuat& Value) override;
    /**
     * @brief FMatrix 값의 네이티브 저장 표현을 그대로 복사합니다.
     * @param Name 형식에서 사용하지 않는 필드 이름입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note sizeof(FMatrix) 바이트를 기록하며 바이트 순서 변환이나 개별 성분 변환을 수행하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FMatrix& Value) override;

    // Scope
    /**
     * @brief 순차 이진 형식에서는 객체 스코프 시작을 기록하지 않습니다.
     * @param Name 사용하지 않는 객체 스코프 이름입니다.
     */
    virtual void BeginObjectScope(std::string_view Name) override;
    /**
     * @brief 순차 이진 형식에서는 객체 스코프 종료를 기록하지 않습니다.
     */
    virtual void EndObjectScope() override;
    /**
     * @brief 배열 원소 수를 Uint32로 기록하거나 읽습니다.
     * @param Name 형식에서 사용하지 않는 배열 이름입니다.
     * @param[in,out] ArraySize 저장할 원소 수 또는 로드한 원소 수입니다.
     * @note 저장 크기가 Uint32 범위를 넘거나 로드한 개수가 남은 바이트 수보다 크면 오류를 설정합니다. 로드 실패 시 ArraySize는 0입니다.
     */
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) override;
    /**
     * @brief 순차 이진 형식에서는 배열 스코프 종료를 기록하지 않습니다.
     */
    virtual void EndArrayScope() override;

private:
    /**
     * @brief 오류 상태와 버퍼 범위를 검사한 뒤 연속 바이트를 복사합니다.
     * @param[in,out] Value 저장할 바이트 또는 로드 결과를 받을 유효한 저장 공간입니다.
     * @param Size 복사할 바이트 수입니다.
     * @note 저장 버퍼 최대 크기를 넘거나 로드 데이터가 부족하면 오류를 설정하고 복사하지 않습니다.
     */
    void SerializeBytes(void* Value, std::size_t Size);

private:
    // Save
    /**
     * @brief 저장 모드에서 끝에 바이트를 추가할 비소유 배열 포인터입니다.
     */
    TArray<Uint8>* mWriteBytes{};

    // Load
    /**
     * @brief 로드 모드의 비소유 입력 배열 포인터입니다.
     */
    const TArray<Uint8>* mReadBytes{};
    /**
     * @brief 로드 버퍼에서 다음에 읽을 바이트 위치입니다.
     */
    std::size_t mReadOffset{};
};
