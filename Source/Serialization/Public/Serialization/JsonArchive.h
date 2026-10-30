/**
 * @file JsonArchive.h
 * @brief RapidJSON을 사용하는 파일 및 값 직렬화를 제공합니다.
 * @ingroup Serialization
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Archive.h"

#include <filesystem>
#include <rapidjson/document.h>

/**
 * @brief RapidJSON 문서와 컨테이너 스택을 이용하는 JSON 아카이브입니다.
 * @ingroup Serialization
 */
class FJsonArchive final : public FArchive {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FJsonArchive, FArchive);

public:
    /**
     * @brief 기반 아카이브의 복합 값 직렬화 오버로드를 노출합니다.
     * @param Name 직렬화할 필드 이름입니다.
     * @param Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    using FArchive::Serialize;

public:
    /**
     * @brief FJsonArchive의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Mode 로드·저장 또는 기즈모 동작 모드입니다.
     */
    explicit FJsonArchive(EArchiveMode Mode);

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    ~FJsonArchive() override;

    /**
     * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
     */
    FJsonArchive(const FJsonArchive&) = delete;

    /**
     * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
     * @return 소유 객체가 유지하는 값의 참조입니다.
     */
    FJsonArchive& operator=(const FJsonArchive&) = delete;

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FJsonArchive);

    /**
     * @brief JSON 파일을 읽어 루트 문서를 교체합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Load(const std::filesystem::path& Path);

    /**
     * @brief JSON 문서를 파일로 저장합니다.
     * @param[in] Path 읽거나 쓸 파일 경로입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Save(const std::filesystem::path& Path);

    /**
     * @brief JSON 텍스트의 구문과 지원 값 범위를 검사하여 문서로 읽습니다.
     * @param[in] Text 파싱하거나 버퍼에 복사할 문자열입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool Parse(const FString& Text);

    /**
     * @brief JSON 문서를 사람이 읽을 수 있는 문자열로 출력합니다.
     * @return 변환된 문자열입니다.
     */
    FString ToString() const;

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
     * @param[in,out] Count 저장할 배열 길이이며 로드 시 읽어 온 배열 길이를 받습니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool BeginArray(std::string_view Name, Uint32& Count) override;

    /**
     * @brief 현재 배열 컨테이너를 끝내고 상위 컨테이너로 돌아갑니다.
     */
    void EndArray() override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, bool& Value) override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, Int32& Value) override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, Uint32& Value) override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, Uint64& Value) override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, float& Value) override;

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in,out] Value 저장 시 읽고 로드 시 덮어쓸 값입니다.
     */
    void Serialize(std::string_view Name, FString& Value) override;

private:
    /**
     * @brief 현재 JSON 컨테이너의 필드나 다음 배열 원소를 조회하고 필요하면 생성합니다.
     * @param[in] Name 조회하거나 기록할 이름입니다.
     * @param[in] Create 필드가 없으면 생성할지 지정합니다.
     * @return 조회한 값의 비소유 주소이며, 해당 값이 없으면 nullptr입니다.
     */
    rapidjson::Value* Find(std::string_view Name, bool Create);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief JSON 루트 문서와 할당기입니다.
     */
    rapidjson::Document mDocument{};
    ReflectProperty(mDocument, "Document", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 현재 JSON 객체·배열의 비소유 탐색 스택입니다.
     */
    TArray<rapidjson::Value*> mStack{};
    ReflectProperty(mStack, "Stack", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
