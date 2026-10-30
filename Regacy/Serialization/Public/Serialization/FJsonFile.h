/**
 * @file FJsonFile.h
 * @brief JSON 객체 파일의 파싱과 임시 파일을 통한 교체 저장을 제공합니다.
 */

#pragma once

#include <filesystem>
#include <rapidjson/document.h>

/// @brief JSON 객체를 파일에서 읽거나 파일로 저장합니다.
class FJsonFile final {
public:
    /// @brief JSON 파일을 읽어 객체로 파싱합니다.
    /// @param[in] FilePath 읽을 JSON 파일 경로.
    /// @param[out] Document 파싱 결과를 저장할 문서. 실패한 경우에도 내용이 변경될 수 있습니다.
    /// @return 파일 읽기와 파싱에 성공하고 루트가 JSON 객체이면 true, 그 외에는 false.
    /// @note JSON 주석과 trailing comma를 허용합니다.
    static bool Load(const std::filesystem::path& FilePath, rapidjson::Document& Document);

    /// @brief JSON 객체를 파일에 저장합니다.
    /// @details 들여쓰기를 적용한 JSON을 임시 파일에 기록한 뒤 대상 파일을 교체합니다.
    /// @param[in] FilePath 저장할 파일 경로. 상위 디렉터리가 없으면 생성합니다.
    /// @param[in] Document 저장할 JSON 문서. 루트는 객체여야 합니다.
    /// @return 대상 파일의 저장 또는 교체에 성공하면 true, 실패하면 false.
    static bool Save(const std::filesystem::path& FilePath, const rapidjson::Document& Document);
};
