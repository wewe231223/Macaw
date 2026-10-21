#pragma once

#include <Windows.h>
#include <source_location>
#include <string_view>

namespace ErrorHandler {
    enum class EErrorLevel {
        Warning,
        Error,
        Critical
    };

    void Initialize();

    void Report(std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());
    void Report(bool Condition, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());

    void ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());
    void ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());

    [[noreturn]] void Fatal(std::string_view Title, std::string_view Message, const std::source_location& Location = std::source_location::current());
}
