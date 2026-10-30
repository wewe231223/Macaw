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
 * @brief 물리 경로를 유지하면서 읽기 쉬운 논리 경로로 표시하는 에셋 항목입니다.
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
     * @param DisplayPath 표시할 논리 경로입니다.
     * @param Type 에셋 종류이며 일반 파일이면 비어 있습니다.
     */
    FEditorAssetEntry(std::filesystem::path Path, std::filesystem::path DisplayPath, std::optional<EAssetType> Type);

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
     * @brief 표시 경로를 반환합니다.
     * @return GUID 저장 경로를 감춘 논리 경로입니다.
     */
    const std::filesystem::path& GetDisplayPath() const;

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
     * @brief 브라우저와 선택기에서 사용하는 논리 경로입니다.
     */
    std::filesystem::path mDisplayPath{};
    ReflectProperty(mDisplayPath, "DisplayPath", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 식별한 에셋 종류입니다.
     */
    std::optional<EAssetType> mType{};
    ReflectProperty(mType, "Type", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};

/**
 * @brief 메시와 현재 연결된 임포트 의존 에셋을 읽기 쉬운 가상 폴더로 제공합니다.
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
     * @brief 디스크와 메시의 참조를 다시 읽습니다.
     * @param Error 읽지 못한 에셋의 설명입니다.
     * @return 폴더 검색을 완료하면 true입니다.
     */
    bool Refresh(FString& Error);

    /**
     * @brief 현재 항목들을 반환합니다.
     * @return 표시 경로 순으로 정렬한 항목 배열입니다.
     */
    const TArray<FEditorAssetEntry>& GetEntries() const;

    /**
     * @brief 표시 폴더들을 반환합니다.
     * @return 내부 저장 폴더를 제외한 폴더 배열입니다.
     */
    const TArray<std::filesystem::path>& GetDirectories() const;

    /**
     * @brief 실제 경로에 해당하는 항목을 조회합니다.
     * @param Path 절대 또는 Content 기준 경로입니다.
     * @return 항목의 비소유 주소이며 없으면 nullptr입니다.
     */
    const FEditorAssetEntry* Find(const std::filesystem::path& Path) const;

    /**
     * @brief 읽기 쉬운 표시 경로를 구합니다.
     * @param Path 실제 에셋 경로입니다.
     * @return 논리 경로이며 미등록 에셋은 파일 이름을 사용합니다.
     */
    FString GetDisplayPath(const std::filesystem::path& Path) const;

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
     * @brief 중복 없이 에셋을 등록합니다.
     * @param Path 실제 에셋 경로입니다.
     * @param DisplayPath 표시 경로입니다.
     * @param Type 에셋 종류입니다.
     */
    void Add(const std::filesystem::path& Path, const std::filesystem::path& DisplayPath, std::optional<EAssetType> Type);

    /**
     * @brief 현재 메시의 재질과 텍스처 참조를 등록합니다.
     * @param Entry 메시 에셋 항목의 사본입니다.
     * @param Error 실패 설명입니다.
     */
    void AddMeshDependencies(const FEditorAssetEntry& Entry, FString& Error);

    /**
     * @brief 임포터의 내부 저장 경로인지 검사합니다.
     * @param Path Content 기준 경로입니다.
     * @return 내부 임포트 저장 경로이면 true입니다.
     */
    static bool IsImportStorage(const std::filesystem::path& Path);

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
     * @brief 표시할 실제·가상 폴더 목록입니다.
     */
    TArray<std::filesystem::path> mDirectories{};
    ReflectProperty(mDirectories, "Directories", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
