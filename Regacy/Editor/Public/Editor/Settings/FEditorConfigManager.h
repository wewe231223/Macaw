/**
 * @file FEditorConfigManager.h
 * @brief 전역 편집 설정 서비스와 설정 파일의 읽기 및 쓰기를 정의합니다.
 */

#pragma once

#include <filesystem>
#include "Editor/Settings/FEditorSettings.h"

/** @brief 프로세스 전체의 FEditorSettings 원본과 해당 설정의 파일 입출력을 제공합니다. @details 편집 UI와 입력 처리는 이 저장소를 직접 조회하며 설정 수정은 자동으로 파일에 저장되지 않습니다. */
class FEditorConfigManager final {
private:
    /** @brief 기본 설정을 보관하는 전역 설정 서비스를 생성합니다. */
    FEditorConfigManager() = default;

public:
    /** @brief 프로세스 종료 시 설정 저장소를 해제합니다. 파일 저장은 수행하지 않습니다. */
    ~FEditorConfigManager() = default;

    /** @brief 전역 설정 서비스의 복사 생성을 금지합니다. */
    FEditorConfigManager(const FEditorConfigManager&) = delete;

    /** @brief 전역 설정 서비스의 복사 대입을 금지합니다. */
    FEditorConfigManager& operator=(const FEditorConfigManager&) = delete;

    /** @brief 전역 설정 서비스의 이동 생성을 금지합니다. */
    FEditorConfigManager(FEditorConfigManager&&) = delete;

    /** @brief 전역 설정 서비스의 이동 대입을 금지합니다. */
    FEditorConfigManager& operator=(FEditorConfigManager&&) = delete;

public:
    /**
     * @brief 프로세스 전체가 공유하는 설정 서비스를 반환합니다.
     * @return 첫 접근 시 기본 설정으로 생성되어 프로세스 종료까지 유지되는 서비스의 참조입니다.
     * @note 최초 생성은 스레드 안전합니다. 설정 읽기, 수정과 파일 입출력의 동기화는 호출자가 관리합니다.
     */
    static FEditorConfigManager& Get();

    /**
     * @brief 공용 설정 원본에 읽기와 쓰기 접근을 제공합니다.
     * @return 편집 UI와 입력 처리가 공유하는 설정의 참조입니다. 수정 내용은 즉시 반영되며 파일 저장에는 Flush가 필요합니다.
     */
    FEditorSettings& GetSettings();

    /**
     * @brief 공용 설정 원본에 읽기 전용 접근을 제공합니다.
     * @return 공유 설정의 상수 참조입니다.
     */
    const FEditorSettings& GetSettings() const;

    /**
     * @brief 파일에서 공용 설정을 다시 읽습니다.
     * @param ConfigPath 읽을 설정 파일 경로입니다.
     * @return 읽기와 역직렬화에 성공하면 true입니다. 실패하면 현재 설정을 유지합니다.
     * @details 파일에 생략된 필드는 기존 값을 유지하며 잘못된 필드가 있으면 변경 사항을 적용하지 않습니다.
     */
    bool Reload(const std::filesystem::path& ConfigPath = "Editor.ini");

    /**
     * @brief 현재 공용 설정을 파일에 저장합니다.
     * @param ConfigPath 저장할 설정 파일 경로입니다.
     * @return 파일 저장에 성공하면 true입니다.
     */
    bool Flush(const std::filesystem::path& ConfigPath = "Editor.ini") const;

private:
    /** @brief 편집 UI와 전역 접근점이 공유하는 설정 원본입니다. */
    FEditorSettings mSettings{};
};
