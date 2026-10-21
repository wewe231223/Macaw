/**
 * @file FArchive.h
 * @brief 로딩과 저장에 공통으로 사용하는 아카이브 인터페이스와 직렬화 헬퍼를 정의한다.
 */
#pragma once
#include "Core/Common.h"
#include "Math/FMath.h"
#include "Core/Base/FGuid.h"
#include "Core/Base/FName.h"
#include "Core/Asset/IAssetResolver.h"

/** @brief 아카이브의 직렬화 방향을 지정한다. */
enum class EArchiveMode : Uint8 {
    /** @brief 아카이브에서 값을 읽는다. */
    Loading,
    /** @brief 아카이브에 값을 기록한다. */
    Saving
};

/**
 * @brief 데이터 형식에 독립적인 로딩 및 저장 인터페이스를 제공한다.
 * @details 파생 클래스는 값과 스코프의 실제 직렬화를 구현한다. 로딩 시 참조 인수에 읽은 값을 반영하고 저장 시 참조 인수의 값을 기록한다.
 * @details Name은 이름 기반 형식에서 필드 이름으로, 배열 스코프에서는 문자열로 변환한 인덱스로 사용된다. 순차 형식에서는 무시할 수 있다.
 */
class FArchive {
public:
    /**
     * @brief 지정한 모드로 아카이브를 생성한다.
     * @param InMode 생성 후 유지할 로딩 또는 저장 모드.
     */
    explicit FArchive(EArchiveMode InMode);

    /** @brief 파생 아카이브를 기본 클래스 포인터로 안전하게 소멸한다. */
    virtual ~FArchive() = default;

public:
    /**
     * @brief 아카이브의 오류 상태를 조회한다.
     * @return 오류가 설정되어 있으면 true. 기본 구현은 SetError()로 설정한 상태를 반환한다.
     */
    virtual bool HasError() const;

    /**
     * @brief 이름으로 필드를 조회할 수 있는지 확인한다.
     * @return 이름 기반 조회를 지원하면 true. 기본 구현은 false를 반환한다.
     */
    virtual bool SupportsNamedFields() const;

    /**
     * @brief 현재 스코프에 지정한 필드가 있는지 조회한다.
     * @param Name 조회할 필드 이름 또는 배열 인덱스 문자열.
     * @return 해당 필드가 있으면 true. 기본 구현은 false를 반환한다.
     * @note SupportsNamedFields()가 true인 아카이브에서 필드 존재 여부를 조회할 수 있다.
     */
    virtual bool HasField(std::string_view Name) const;

    /** @brief 기본 아카이브의 오류 상태를 설정한다. 설정한 상태는 이 인터페이스로 해제할 수 없다. */
    void SetError();

    /**
     * @brief 아카이브가 로딩 모드인지 확인한다.
     * @return 모드가 EArchiveMode::Loading이면 true.
     */
    bool IsLoading() const;

    /**
     * @brief 아카이브가 저장 모드인지 확인한다.
     * @return 모드가 EArchiveMode::Saving이면 true.
     */
    bool IsSaving() const;

    // ---------------------------------------------------
    // 1. 기본 원시 타입 (Primitives)
    // ---------------------------------------------------
    /**
     * @brief 불리언 값을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, bool& Value) = 0;

    /**
     * @brief 8비트 부호 없는 정수를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Uint8& Value) = 0;

    /**
     * @brief 32비트 부호 있는 정수를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Int32& Value) = 0;

    /**
     * @brief 32비트 부호 없는 정수를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Uint32& Value) = 0;

    /**
     * @brief 64비트 부호 있는 정수를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Int64& Value) = 0;

    /**
     * @brief 64비트 부호 없는 정수를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Uint64& Value) = 0;

    /**
     * @brief 32비트 부동소수점 값을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Float32& Value) = 0;

    /**
     * @brief 64비트 부동소수점 값을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, Float64& Value) = 0;

    // ---------------------------------------------------
    // 2. 수학 코어 타입 (DirectX SimpleMath)
    // ---------------------------------------------------
    /**
     * @brief 2차원 벡터를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FVector2& Value) = 0;

    /**
     * @brief 3차원 벡터를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FVector& Value) = 0;

    /**
     * @brief 4차원 벡터를 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FVector4& Value) = 0;

    /**
     * @brief 쿼터니언을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FQuat& Value) = 0;

    /**
     * @brief 행렬을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FMatrix& Value) = 0;

    // ---------------------------------------------------
    // 3. 엔진 코어 타입
    // ---------------------------------------------------
    /**
     * @brief 문자열을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FString& Value) = 0;

    /**
     * @brief GUID 값을 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 값 또는 로딩한 값을 받을 변수.
     */
    virtual void Serialize(std::string_view Name, FGuid& Value) = 0;

    /**
     * @brief 배열 크기와 각 요소를 직렬화한다.
     * @tparam T Serialize()로 직렬화할 수 있는 요소 타입.
     * @param Name 배열 필드 이름 또는 상위 배열의 인덱스 문자열.
     * @param[in,out] ArrayValue 저장할 배열 또는 로딩한 요소를 받을 배열.
     * @details 로딩 시 읽은 크기로 배열을 조정하고 각 요소를 인덱스 문자열로 직렬화한다. 오류가 감지되면 요소 처리를 중단하고 배열 스코프를 닫는다.
     */
    template <typename T>
    void Serialize(std::string_view Name, TArray<T>& ArrayValue);

    /**
     * @brief 이름을 문자열로 변환하여 직렬화한다.
     * @param Name 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 이름 또는 로딩한 이름을 받을 변수.
     * @details 로딩 중 오류가 발생하거나 문자열 길이가 NameSize 이상이면 오류를 설정하고 Value를 유지한다.
     */
    void Serialize(std::string_view Name, FName& Value);

    /**
     * @brief 객체 식별 GUID를 직렬화하고 로딩 시 재매핑한다.
     * @param Name GUID 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] Value 저장할 GUID 또는 로딩 및 재매핑한 GUID를 받을 변수.
     * @details 로딩 중 오류가 없으면 등록된 재매핑 테이블을 적용한다. 매핑이 없거나 대상 GUID가 유효하지 않으면 오류를 설정하고 Value를 비운다.
     */
    void SerializeObjectGuid(std::string_view Name, FGuid& Value);

    /**
     * @brief 객체 참조 GUID를 등록된 테이블로 재매핑한다.
     * @param[in,out] Guid 재매핑할 참조 GUID. 실패하면 비워진다.
     * @return Guid가 유효하지 않거나 재매핑 테이블이 없거나 재매핑에 성공하면 true. 매핑이 없거나 대상 GUID가 유효하지 않으면 오류를 설정하고 false를 반환한다.
     * @details 로딩 및 저장 모드와 관계없이 호출할 수 있다.
     */
    bool RemapObjectReference(FGuid& Guid);

    /**
     * @brief 객체 GUID 재매핑 테이블을 등록한다.
     * @param Remapping 원본 GUID에서 대상 GUID로의 매핑. nullptr이면 재매핑을 해제한다.
     * @details 테이블을 복사하거나 소유하지 않는다. 등록한 테이블은 아카이브가 사용하는 동안 유효해야 한다.
     */
    void SetObjectGuidRemapping(const TMap<FGuid, FGuid>* Remapping);

    /**
     * @brief 등록된 객체 GUID 재매핑 테이블을 조회한다.
     * @return 외부에서 소유하는 테이블 포인터. 등록되지 않았으면 nullptr.
     */
    const TMap<FGuid, FGuid>* GetObjectGuidRemapping() const;

    // ---------------------------------------------------
    // 4. 구조체 / 계층 처리 (Scope)
    // ---------------------------------------------------
    /**
     * @brief 중첩 객체의 직렬화 스코프를 시작한다.
     * @param Name 객체 필드 이름 또는 배열 인덱스 문자열.
     * @details 스코프의 표현은 파생 클래스가 결정한다. 시작한 스코프는 EndObjectScope()로 닫아야 한다.
     */
    virtual void BeginObjectScope(std::string_view Name) = 0;

    /** @brief 현재 객체 스코프를 닫고 상위 스코프로 돌아간다. */
    virtual void EndObjectScope() = 0;

    // ---------------------------------------------------
    // 5. 배열 처리 (Scope)
    // ---------------------------------------------------
    /**
     * @brief 배열의 직렬화 스코프를 시작하고 요소 수를 처리한다.
     * @param Name 배열 필드 이름 또는 상위 배열의 인덱스 문자열.
     * @param[in,out] ArraySize 저장 시 기록할 요소 수, 로딩 시 읽은 요소 수를 받을 변수.
     * @details 시작한 스코프는 EndArrayScope()로 닫아야 한다.
     */
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) = 0;

    /** @brief 현재 배열 스코프를 닫고 상위 스코프로 돌아간다. */
    virtual void EndArrayScope() = 0;

    // ---------------------------------------------------
    // 6. 구조체 직렬화 헬퍼 (UObject 자식들이나 커스텀 구조체)
    // ---------------------------------------------------
    /**
     * @brief 객체 스코프 안에서 사용자 정의 타입을 직렬화한다.
     * @tparam T Serialize(FArchive&) 멤버 함수를 제공하는 타입.
     * @param Name 객체 필드 이름 또는 배열 인덱스 문자열.
     * @param[in,out] StructValue 저장할 객체 또는 로딩한 값을 받을 객체.
     * @details BeginObjectScope(), StructValue.Serialize(*this), EndObjectScope()를 차례로 호출한다.
     */
    template <typename T>
    void SerializeStruct(std::string_view Name, T& StructValue);

    // ---------------------------------------------------
    // 7. Asset Registry 처리
    // ---------------------------------------------------
    /**
     * @brief 에셋 참조를 해석할 외부 리졸버를 등록한다.
     * @param InAssetResolver 사용할 리졸버. nullptr이면 등록을 해제한다.
     * @details 리졸버를 소유하지 않는다. 등록한 리졸버는 아카이브가 사용하는 동안 유효해야 한다.
     */
    void SetAssetResolver(const IAssetResolver* InAssetResolver);

    /**
     * @brief 등록된 에셋 리졸버를 조회한다.
     * @return 외부에서 소유하는 리졸버 포인터. 등록되지 않았으면 nullptr.
     */
    const IAssetResolver* GetAssetResolver();

private:
    /**
     * @brief GUID 재매핑을 수행하고 실패 시 오류 상태를 설정한다.
     * @param[in,out] Guid 재매핑할 GUID. 실패하면 비워진다.
     * @return 테이블이 없거나 유효한 대상 GUID로 재매핑하면 true. 그 외에는 false.
     */
    bool RemapObjectGuid(FGuid& Guid);

private:
    /** @brief 생성 시 결정되는 직렬화 모드. */
    const EArchiveMode mMode{};

    /** @brief 외부에서 소유하는 에셋 리졸버. */
    const IAssetResolver* mAssetResolver{};

    /** @brief SetError()로 설정되는 기본 오류 상태. */
    bool mError{};

    /** @brief 외부에서 소유하는 객체 GUID 재매핑 테이블. */
    const TMap<FGuid, FGuid>* mObjectGuidRemapping{};
};

template <typename T>
void FArchive::Serialize(std::string_view Name, TArray<T>& ArrayValue) {
    std::size_t Size{ArrayValue.size()};

    BeginArrayScope(Name, Size);

    if (HasError()) {
        EndArrayScope();
        return;
    }

    if (IsLoading()) {
        ArrayValue.resize(Size);
    }

    for (std::size_t I{0}; I < Size && !HasError(); ++I) {
        // 인덱스를 Name으로 변환하여 순회
        Serialize(std::to_string(I), ArrayValue[I]);
    }

    EndArrayScope();
}

template <typename T>
void FArchive::SerializeStruct(std::string_view Name, T& StructValue) {
    BeginObjectScope(Name);
    StructValue.Serialize(*this);
    EndObjectScope();
}
