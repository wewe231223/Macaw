/**
 * @file FAssetImportScope.h
 * @brief 임포트 실패 시 새로 만든 의존 에셋을 정리합니다.
 * @ingroup Engine
 */
#pragma once

#include "Core/ReflectionMacros.h"

#include <filesystem>

/**
 * @brief 한 번의 임포트에 전용인 새 디렉터리를 성공 확정 전까지 관리합니다.
 */
class FAssetImportScope {
public:
    /**
     * @brief 리플렉션 소유 타입의 별칭입니다.
     */
    ReflectTypes(FAssetImportScope);

public:
    /**
     * @brief 소유할 새 디렉터리 경로를 저장합니다.
     * @param Directory GUID로 구분한 이번 임포트 전용 경로입니다.
     */
    explicit FAssetImportScope(std::filesystem::path Directory);
    /**
     * @brief 성공 확정 전이라면 이번 임포트가 만든 파일을 제거합니다.
     */
    ~FAssetImportScope();
    /**
     * @brief 롤백 소유권의 복사를 금지합니다.
     * @param Other 복사 대상입니다.
     */
    FAssetImportScope(const FAssetImportScope& Other) = delete;
    /**
     * @brief 롤백 소유권의 복사를 금지합니다.
     * @param Other 대입 대상입니다.
     * @return 이 객체의 참조입니다.
     */
    FAssetImportScope& operator=(const FAssetImportScope& Other) = delete;

public:
    /**
     * @brief 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 정보입니다.
     */
    ReflectStruct(FAssetImportScope);
    /**
     * @brief 임포트가 완성되어 의존 에셋을 유지하도록 표시합니다.
     */
    void Commit();

private:
    /**
     * @brief 생성된 리플렉션 등록 함수에 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 이번 임포트만 사용하는 의존 에셋 디렉터리입니다.
     */
    std::filesystem::path mDirectory{};
    ReflectProperty(mDirectory, "Directory", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 메시 에셋 저장까지 성공했는지 여부입니다.
     */
    bool mCommitted{};
    ReflectProperty(mCommitted, "Committed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
