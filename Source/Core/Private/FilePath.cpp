/**
 * @file FilePath.cpp
 * @brief 파일 시스템 경로와 UTF-8 문자열의 변환을 구현합니다.
 * @ingroup Core
 */

#include "Core/FilePath.h"

#include <algorithm>
#include <cwctype>
#include <utility>

namespace Core {

    Core::FString Core::FilePath::ToUtf8(const std::filesystem::path& Path) {
        const std::u8string Text{Path.u8string()};
        return Core::FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }

    Core::FString Core::FilePath::ToGenericUtf8(const std::filesystem::path& Path) {
        const std::u8string Text{Path.generic_u8string()};
        return Core::FString{reinterpret_cast<const char*>(Text.data()), Text.size()};
    }

    std::filesystem::path Core::FilePath::FromUtf8(std::string_view Text) {
        return std::filesystem::path{std::u8string_view{reinterpret_cast<const char8_t*>(Text.data()), Text.size()}};
    }

    std::filesystem::path Core::FilePath::ToLower(const std::filesystem::path& Path) {
        std::wstring Text{Path.wstring()};
        std::transform(Text.begin(), Text.end(), Text.begin(), [](wchar_t Character) {
            return static_cast<wchar_t>(std::towlower(Character));
        });
        return std::filesystem::path{std::move(Text)};
    }

    std::wstring Core::FilePath::ToLowerExtension(const std::filesystem::path& Path) {
        return ToLower(Path.extension()).wstring();
    }

}
