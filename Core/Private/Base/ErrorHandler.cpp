#include "pch.h"
#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <new>
#include <sstream>
#include <string>

#ifdef ENABLE_ASSERTION_STACKTRACE
#include "cpptrace/cpptrace.hpp"
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

namespace ErrorHandler {
    namespace {
        std::ofstream GLogFile{};
        thread_local bool ReportingCritical{};
        thread_local char CriticalTitle[257]{};
        thread_local char CriticalMessage[3501]{};

        [[noreturn]] void ReportCriticalFallback() noexcept {
            char Buffer[4096]{};
            const int Length{std::snprintf(Buffer, sizeof(Buffer), "[CRITICAL] %s\n%s\n", CriticalTitle, CriticalMessage)};

            if (Length > 0) {
#ifdef _WIN32
                OutputDebugStringA(Buffer);
                CreateDirectoryW(L"Log", nullptr);

                const HANDLE File{CreateFileW(L"Log/critical.log", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};

                if (File != INVALID_HANDLE_VALUE) {
                    DWORD Written{};

                    WriteFile(File, Buffer, static_cast<DWORD>((std::min)(Length, static_cast<int>(sizeof(Buffer) - 1))), &Written, nullptr);
                    FlushFileBuffers(File);
                    CloseHandle(File);
                }

                MessageBoxA(nullptr, Buffer, "Critical error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
#else
                std::fputs(Buffer, stderr);
                std::fflush(stderr);
#endif
            }

            std::_Exit(EXIT_FAILURE);
        }

        [[noreturn]] void HandleUnexpectedTermination() noexcept {
            Report("Unhandled exception", "An unhandled exception or invalid termination state prevents the program from continuing.", EErrorLevel::Critical);
            std::_Exit(EXIT_FAILURE);
        }

        [[noreturn]] void HandleAllocationFailure() {
            Report("Memory allocation", "The system could not allocate the requested memory.", EErrorLevel::Critical);
            std::_Exit(EXIT_FAILURE);
        }

        void EnsureLogFileOpen() {
            if (GLogFile.is_open()) {
                return;
            }

            std::error_code Error{};

            std::filesystem::create_directories("Log", Error);

            if (Error) {
                return;
            }

            auto Now{std::chrono::current_zone()->to_local(std::chrono::system_clock::now())};
            std::string FileName{std::format("Log/log_{:%Y%m%d_%H%M%S}.txt", Now)};

            GLogFile.open(FileName, std::ios::out | std::ios::app);

            if (GLogFile) {
                GLogFile << std::format("=== Log Started at {:%Y-%m-%d %H:%M:%S} ===\n\n", Now);
            }
        }

        std::string GetDetailedStackTrace() {
            std::stringstream Stream{};

            Stream << "\n=== STACK TRACE ===\n";

#ifdef ENABLE_ASSERTION_STACKTRACE
            cpptrace::generate_trace().print(Stream, false);
#else
            Stream << "Stack trace not available. Enable ENABLE_ASSERTION_STACKTRACE for detailed stack traces.\n";
#endif

            Stream << "===================\n";

            return Stream.str();
        }

#ifdef _WIN32

        std::string WideToUTF8(std::wstring_view Text) {
            if (Text.empty()) {
                return {};
            }

            int RequiredSize{WideCharToMultiByte(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), nullptr, 0, nullptr, nullptr)};

            if (RequiredSize <= 0) {
                return {};
            }

            std::string Result{};

            Result.resize(static_cast<std::size_t>(RequiredSize), '\0');

            WideCharToMultiByte(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), Result.data(), RequiredSize, nullptr, nullptr);

            return Result;
        }

        std::wstring UTF8ToWide(std::string_view Text) {
            if (Text.empty()) {
                return {};
            }

            int RequiredSize{MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), nullptr, 0)};

            if (RequiredSize <= 0) {
                return {};
            }

            std::wstring Result{};

            Result.resize(static_cast<std::size_t>(RequiredSize), L'\0');

            MultiByteToWideChar(CP_UTF8, 0, Text.data(), static_cast<int>(Text.size()), Result.data(), RequiredSize);

            return Result;
        }

        void TrimSystemMessage(std::wstring& Message) {
            while (!Message.empty() && (Message.back() == L'\r' || Message.back() == L'\n' || Message.back() == L' ')) {
                Message.pop_back();
            }
        }

        std::string GetHRESULTMessage(HRESULT Result) {
            LPWSTR MessageBuffer{nullptr};
            DWORD Flags{FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS};
            DWORD MessageID{static_cast<DWORD>(Result)};

            DWORD Length{FormatMessageW(Flags, nullptr, MessageID, 0, reinterpret_cast<LPWSTR>(&MessageBuffer), 0, nullptr)};

            if (Length == 0 && HRESULT_FACILITY(Result) == FACILITY_WIN32) {
                MessageID = HRESULT_CODE(Result);
                Length = FormatMessageW(Flags, nullptr, MessageID, 0, reinterpret_cast<LPWSTR>(&MessageBuffer), 0, nullptr);
            }

            if (Length == 0 || MessageBuffer == nullptr) {
                return "No system error message is available.";
            }

            std::wstring Message{MessageBuffer, Length};

            LocalFree(MessageBuffer);

            TrimSystemMessage(Message);

            return WideToUTF8(Message);
        }

        std::string BuildHRESULTMessage(HRESULT Result, std::string_view Message, const std::source_location& Location) {
            std::uint32_t ResultValue{static_cast<std::uint32_t>(Result)};

            return std::format(
                "{}\n\n"
                "HRESULT : 0x{:08X}\n"
                "Facility: {}\n"
                "Code    : {}\n"
                "System  : {}\n\n"
                "Location:\n"
                "{}:{}\n"
                "{}",
                Message,
                ResultValue,
                HRESULT_FACILITY(Result),
                HRESULT_CODE(Result),
                GetHRESULTMessage(Result),
                Location.file_name(),
                Location.line(),
                Location.function_name());
        }

        void ShowErrorMessageBox(std::string_view Title, std::string_view Message) {
            std::wstring WideTitle{UTF8ToWide(Title)};
            std::wstring WideMessage{UTF8ToWide(Message)};

            MessageBoxW(nullptr, WideMessage.c_str(), WideTitle.c_str(), MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
        }

#endif
    }

    void Initialize() {
        std::set_terminate(HandleUnexpectedTermination);
        std::set_new_handler(HandleAllocationFailure);
    }

    void Report(std::string_view Title, std::string_view Message, EErrorLevel Level) {
        if (Level == EErrorLevel::Critical) {
            if (ReportingCritical) {
                ReportCriticalFallback();
            }

            ReportingCritical = true;

            const std::size_t TitleLength{(std::min)(Title.size(), sizeof(CriticalTitle) - 1)};
            const std::size_t MessageLength{(std::min)(Message.size(), sizeof(CriticalMessage) - 1)};

            std::copy_n(Title.data(), TitleLength, CriticalTitle);
            std::copy_n(Message.data(), MessageLength, CriticalMessage);
            CriticalTitle[TitleLength] = '\0';
            CriticalMessage[MessageLength] = '\0';
        }

        EnsureLogFileOpen();

        auto Now{std::chrono::current_zone()->to_local(std::chrono::system_clock::now())};

        std::string_view LevelString{"UNKNOWN"};

        switch (Level) {
            case EErrorLevel::Warning:
                LevelString = "WARNING";
                break;

            case EErrorLevel::Error:
                LevelString = "ERROR";
                break;

            case EErrorLevel::Critical:
                LevelString = "CRITICAL";
                break;

            default:
                LevelString = "UNKNOWN";
                break;
        }

        std::string LogContent{std::format("----------------------------------------\n"
                                           "[{:%H:%M:%S}] [{}] {}\n"
                                           "Message: {}\n",
                                           Now, LevelString, Title, Message)};

        if (Level == EErrorLevel::Critical) {
            LogContent += GetDetailedStackTrace();
        }

        if (GLogFile.is_open()) {
            GLogFile << LogContent;
            GLogFile.flush();
        }

#ifdef _WIN32
        if (Level == EErrorLevel::Error || Level == EErrorLevel::Critical) {
            ShowErrorMessageBox(Title, Message);
        }
#endif

        if (Level == EErrorLevel::Critical) {
            std::_Exit(EXIT_FAILURE);
        }
    }

    void Report(bool BCondition, std::string_view Title, std::string_view Message, EErrorLevel Level) {
        if (BCondition) {
            Report(Title, Message, Level);
        }
    }

#ifdef _WIN32

    void ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
        if (SUCCEEDED(Result)) {
            return;
        }

        Report(Title, BuildHRESULTMessage(Result, Message, Location), Level);
    }

    void ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
        if (ErrorCode == ERROR_SUCCESS) {
            return;
        }

        HRESULT Result{HRESULT_FROM_WIN32(ErrorCode)};

        ReportHRESULT(Result, Title, Message, Level, Location);
    }

#endif
}
