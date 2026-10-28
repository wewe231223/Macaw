/**
 * @file FilePath.h
 * @brief UTF-8 문자열과 파일 시스템 경로 사이의 공통 변환을 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Types.h"

#include <filesystem>
#include <string_view>

/** @brief 경로의 네이티브 구분자와 공통 구분자를 구분하여 UTF-8로 변환합니다. */
namespace FilePath {
    /** @brief 네이티브 경로 구분자를 유지한 UTF-8 문자열을 반환합니다. @param[in] Path 변환할 경로입니다. @return 네이티브 형식의 UTF-8 경로입니다. */
    FString ToUtf8(const std::filesystem::path& Path);
    /** @brief 경로 구분자를 슬래시로 통일한 UTF-8 문자열을 반환합니다. @param[in] Path 변환할 경로입니다. @return 공통 형식의 UTF-8 경로입니다. */
    FString ToGenericUtf8(const std::filesystem::path& Path);

    /** @brief 길이가 지정된 UTF-8 문자열을 파일 시스템 경로로 변환합니다. @param[in] Text 변환할 UTF-8 경로입니다. @return 플랫폼의 파일 시스템 경로입니다. */
    std::filesystem::path FromUtf8(std::string_view Text);
}
