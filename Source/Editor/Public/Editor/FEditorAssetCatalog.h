/**
 * @file FEditorAssetCatalog.h
 * @brief 실제 에셋 경로와 사용자에게 표시할 폴더·이름의 목록입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Engine/FAssetFile.h"

#include <optional>

class FAssetManager;

/**
 * @brief Content 상대 경로와 종류를 가진 에셋 항목입니다.
 */
class FEditorAssetEntry {
public:
    /**
     * @brief 리플렉션 소유 타입입니다.
     */
    ReflectTypes(FEditorAssetEntry);

public:
    /**
     * @brief 에셋 항목을 만듭니다.
     * @param Path Content 기준 실제 경로입니다.
     * @param Type 에셋 종류이며 일반 파일이면 비어 있습니다.
     */
    FEditorAssetEntry(std::filesystem::path Path, std::optional<EAssetType> Type);

public:
    /**
     * @brief 정적 메타데이터를 반환합니다.
     * @return 등록한 구조체 정보입니다.
     */
    ReflectStruct(FEditorAssetEntry);

    /**
     * @brief 실제 참조 경로를 반환합니다.
     * @return Content 기준 경로의 참조입니다.
     */
    const std::filesystem::path& GetPath() const;

    /**
     * @brief 에셋 종류를 반환합니다.
     * @return 에셋 종류이며 일반 파일이면 비어 있습니다.
     */
    std::optional<EAssetType> GetType() const;

private:
    /**
     * @brief 자동 멤버 등록 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 저장·로드·드래그에 사용하는 실제 경로입니다.
     */
    std::filesystem::path mPath{};
    ReflectProperty(mPath, "Path", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 식별한 에셋 종류입니다.
     */
    std::optional<EAssetType> mType{};
    ReflectProperty(mType, "Type", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief Content의 실제 폴더와 에셋 목록을 제공합니다.
 */
class FEditorAssetCatalog {
public:
    /**
     * @brief 리플렉션 소유 타입입니다.
     */
    ReflectTypes(FEditorAssetCatalog);

public:
    /**
     * @brief 프로젝트 관리자를 연결합니다.
     * @param Assets 프로젝트 에셋 관리자의 비소유 주소입니다.
     */
    explicit FEditorAssetCatalog(FAssetManager* Assets);

public:
    /**
     * @brief 정적 메타데이터를 반환합니다.
     * @return 등록한 구조체 정보입니다.
     */
    ReflectStruct(FEditorAssetCatalog);

    /**
     * @brief Content의 폴더와 파일을 다시 읽습니다.
     * @param Error 읽지 못한 에셋의 설명입니다.
     * @return 폴더 검색을 완료하면 true입니다.
     */
    bool Refresh(FString& Error);

    /**
     * @brief 현재 항목들을 반환합니다.
     * @return 실제 경로 순으로 정렬한 항목 배열입니다.
     */
    const TArray<FEditorAssetEntry>& GetEntries() const;

    /**
     * @brief 표시 폴더들을 반환합니다.
     * @return Content 아래의 실제 폴더 배열입니다.
     */
    const TArray<std::filesystem::path>& GetDirectories() const;

    /**
     * @brief 실제 경로에 해당하는 항목을 조회합니다.
     * @param Path 절대 또는 Content 기준 경로입니다.
     * @return 항목의 비소유 주소이며 없으면 nullptr입니다.
     */
    const FEditorAssetEntry* Find(const std::filesystem::path& Path) const;

    /**
     * @brief 프로젝트 에셋 관리자를 반환합니다.
     * @return 생성 때 연결한 비소유 주소입니다.
     */
    FAssetManager* GetAssets() const;

private:
    /**
     * @brief 자동 멤버 등록 접근을 허용합니다.
     */
    ReflectMembers();

    /**
     * @brief 비교용 경로 키를 만듭니다.
     * @param Path Content 기준 경로입니다.
     * @return 대소문자를 정규화한 경로 키입니다.
     */
    static FString MakeKey(const std::filesystem::path& Path);

private:
    /**
     * @brief 프로젝트 에셋 관리자입니다.
     */
    FAssetManager* mAssets{};
    ReflectProperty(mAssets, "Assets", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 브라우저와 선택기가 공유하는 표시 목록입니다.
     */
    TArray<FEditorAssetEntry> mEntries{};
    ReflectProperty(mEntries, "Entries", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 실제 경로별 항목 인덱스입니다.
     */
    TMap<FString, std::size_t> mIndices{};
    ReflectProperty(mIndices, "Indices", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 표시할 실제 폴더 목록입니다.
     */
    TArray<std::filesystem::path> mDirectories{};
    ReflectProperty(mDirectories, "Directories", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
