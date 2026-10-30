/**
 * @file FArchiveJson.h
 * @brief RapidJSON DOM을 사용하는 이름 기반 저장·로드 아카이브를 제공합니다.
 */

#pragma once
#include "Core/Archive/FArchive.h"
#include <rapidjson/document.h>

/**
 * @brief 외부 JSON 객체 또는 배열을 스코프 스택으로 탐색하는 아카이브입니다.
 * @note 로드 시 없는 필드는 기본적으로 기존 값을 유지하고, 존재하는 필드의 잘못된 타입은 오류로 표시합니다.
 * @note FName과 템플릿 값·배열을 처리하는 FArchive의 Serialize 오버로드도 함께 공개합니다.
 * @pre 루트는 객체 또는 배열이어야 하며 노드와 저장용 할당자는 아카이브 사용 중 유효해야 합니다. Begin과 End 스코프를 균형 있게 호출해야 합니다.
 */
class FArchiveJson : public FArchive {
public:
    using FArchive::Serialize;

public:
    /**
     * @brief 루트 노드와 할당자로 JSON 저장 아카이브를 생성합니다.
     * @param RootNode 필드를 추가할 외부 JSON 객체 또는 배열입니다.
     * @param InAllocator 루트 문서와 새 노드에서 사용할 할당자입니다.
     */
    FArchiveJson(rapidjson::Value& RootNode, rapidjson::Document::AllocatorType& InAllocator);

    /**
     * @brief 기존 루트 노드를 탐색할 JSON 로드 아카이브를 생성합니다.
     * @param RootNode 읽을 외부 JSON 객체 또는 배열입니다.
     */
    FArchiveJson(rapidjson::Value& RootNode);

public:
    /**
     * @brief JSON의 이름 기반 필드 조회 지원 여부를 반환합니다.
     * @return 항상 true입니다.
     */
    bool SupportsNamedFields() const override;

    /**
     * @brief 현재 스코프에서 이름 또는 배열 인덱스로 자식 노드 존재 여부를 검사합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @return 유효한 현재 스코프에 자식이 있으면 true입니다.
     */
    bool HasField(std::string_view Name) const override;

    // Primitives
    /**
     * @brief 불리언을 JSON bool로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 존재하는 로드 필드가 bool이 아니면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, bool& Value) override;

    /**
     * @brief 바이트 값을 JSON 부호 없는 정수로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 Uint32로 읽고 Uint8 최대값을 넘으면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Uint8& Value) override;

    /**
     * @brief Int32 값을 JSON 정수로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 해당 Int32 범위를 지원하는 JSON 정수 타입인지 검사하고 불일치이면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Int32& Value) override;

    /**
     * @brief Uint32 값을 JSON 정수로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 해당 Uint32 범위를 지원하는 JSON 정수 타입인지 검사하고 불일치이면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Uint32& Value) override;

    /**
     * @brief Int64 값을 JSON 정수로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 해당 Int64 범위를 지원하는 JSON 정수 타입인지 검사하고 불일치이면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Int64& Value) override;

    /**
     * @brief Uint64 값을 JSON 정수로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 해당 Uint64 범위를 지원하는 JSON 정수 타입인지 검사하고 불일치이면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Uint64& Value) override;

    /**
     * @brief 단정밀도 값을 JSON 수치로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 double로 읽어 유한성과 Float32 범위를 검사하고 위반하면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Float32& Value) override;

    /**
     * @brief 배정밀도 값을 JSON 수치로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 존재하는 로드 필드가 JSON 수치가 아니면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, Float64& Value) override;

    // Engine Core
    /**
     * @brief 문자열 길이를 보존해 JSON 문자열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 JSON 문자열이 아니면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, FString& Value) override;

    /**
     * @brief GUID를 문자열로 직렬화하고 로드 시 파싱합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note GUID 문자열을 파싱할 수 없으면 오류를 설정합니다.
     */
    virtual void Serialize(std::string_view Name, FGuid& Value) override;

    // Math Types
    /**
     * @brief 벡터를 X, Y 순서의 숫자 2개 배열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 배열 크기와 모든 성분의 수치 타입·유한성·Float32 범위를 검사한 뒤 값을 반영합니다.
     */
    virtual void Serialize(std::string_view Name, FVector2D& Value) override;

    /**
     * @brief 벡터를 X, Y, Z 순서의 숫자 3개 배열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 배열 크기와 모든 성분의 수치 타입·유한성·Float32 범위를 검사한 뒤 값을 반영합니다.
     */
    virtual void Serialize(std::string_view Name, FVector3& Value) override;

    /**
     * @brief 벡터를 X, Y, Z, W 순서의 숫자 4개 배열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 배열 크기와 모든 성분의 수치 타입·유한성·Float32 범위를 검사한 뒤 값을 반영합니다.
     */
    virtual void Serialize(std::string_view Name, FVector4& Value) override;

    /**
     * @brief 쿼터니언을 X, Y, Z, W 순서의 숫자 4개 배열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note FVector4 직렬화의 크기·성분 검사를 사용하며 정규화하지 않습니다.
     */
    virtual void Serialize(std::string_view Name, FQuat& Value) override;

    /**
     * @brief 4×4 행렬을 행 우선 순서의 숫자 16개 배열로 직렬화합니다.
     * @param Name 객체 키 또는 배열의 숫자 인덱스 문자열입니다.
     * @param[in,out] Value 저장할 값 또는 로드 결과입니다.
     * @note 로드 시 배열 크기와 모든 성분의 수치 타입·유한성·Float32 범위를 검사합니다.
     */
    virtual void Serialize(std::string_view Name, FMatrix& Value) override;

    // Scope
    /**
     * @brief 하위 JSON 객체를 추가하거나 조회해 현재 스코프로 진입합니다.
     * @param Name 객체 부모에서는 키, 배열 부모의 로드에서는 숫자 인덱스입니다.
     * @note 로드 시 누락된 객체는 null 스코프로 진입하고 읽기를 건너뜁니다. 자식이 존재하지만 객체가 아니면 오류도 설정합니다.
     */
    virtual void BeginObjectScope(std::string_view Name) override;

    /**
     * @brief 객체 스코프를 제거하고 부모 스코프로 돌아갑니다.
     * @pre 대응하는 BeginObjectScope 호출이 있어야 합니다. 루트 스코프를 제거한 뒤 직렬화를 호출하면 안 됩니다.
     */
    virtual void EndObjectScope() override;

    // Array
    /**
     * @brief 하위 배열을 추가하거나 조회해 배열 스코프로 진입합니다.
     * @param Name 객체 부모에서는 키, 배열 부모의 로드에서는 숫자 인덱스입니다.
     * @param[in,out] ArraySize 로드 시 실제 원소 수를 받습니다. 저장 시 인수로 배열을 미리 채우지는 않습니다.
     * @note 유효한 부모에서 로드 배열이 누락되거나 타입이 다르면 ArraySize는 0이며 null 스코프로 진입합니다. 타입 불일치에는 오류도 설정합니다.
     */
    virtual void BeginArrayScope(std::string_view Name, std::size_t& ArraySize) override;

    /**
     * @brief 배열 스코프를 제거하고 부모 스코프로 돌아갑니다.
     * @pre 대응하는 BeginArrayScope 호출이 있어야 합니다.
     */
    virtual void EndArrayScope() override;

private:
    // called when saving in progress
    /**
     * @brief 저장 모드에서 부모 객체에 필드를 추가하거나 부모 배열 끝에 자식을 추가합니다.
     * @param Parent 유효한 객체 또는 배열 부모입니다.
     * @param Name 객체에 사용할 키입니다. 배열 부모에서는 사용하지 않습니다.
     * @param[in,out] Child 부모에 이동할 JSON 값입니다.
     * @pre 저장용 할당자가 설정되어 있어야 합니다.
     */
    void AddChildNode(rapidjson::Value* Parent, std::string_view Name, rapidjson::Value& Child);

    // called when loading in progress
    /**
     * @brief 부모의 객체 키 또는 배열 인덱스로 자식을 찾습니다.
     * @param Parent 조회할 유효한 부모 노드입니다.
     * @param Name 객체 키 또는 std::stoull로 읽을 배열 인덱스 문자열입니다.
     * @pre 배열 인덱스는 변환 가능한 비음수 숫자 문자열이어야 합니다. 변환 실패 예외는 이 함수에서 처리하지 않습니다.
     * @return 해당 자식입니다. 없거나 인덱스가 범위를 벗어나면 nullptr입니다.
     */
    rapidjson::Value* GetChildNode(rapidjson::Value* Parent, std::string_view Name) const;

private:
    /**
     * @brief 저장 노드를 할당하는 비소유 할당자 포인터입니다. 로드 모드에서는 nullptr입니다.
     */
    rapidjson::Document::AllocatorType* mAllocator{};

    /**
     * @brief 루트부터 현재 스코프까지의 비소유 노드 포인터 스택입니다. 누락된 로드 스코프는 nullptr로 표현합니다.
     */
    TArray<rapidjson::Value*> mNodeStack{};
};
