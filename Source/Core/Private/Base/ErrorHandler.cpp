#include "Core/Base/ErrorHandler.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <new>
#include <span>
#include <string>

namespace {
    std::ofstream LogFile{};
    thread_local bool ReportingCritical{};
    thread_local std::array<char, 257> CriticalTitle{};
    thread_local std::array<char, 3501> CriticalMessage{};

    void Append(std::span<char> Buffer, std::size_t& Length, std::string_view Text) {
        const std::size_t Count{std::min(Text.size(), Buffer.size() - Length - 1)};
        std::copy_n(Text.begin(), Count, Buffer.begin() + Length);
        Length += Count;
        Buffer[Length] = '\0';
    }

    [[noreturn]] void ReportCriticalFallback() noexcept {
        std::array<char, 4096> Buffer{};
        std::size_t Length{};

        Append(Buffer, Length, "[CRITICAL] ");
        Append(Buffer, Length, CriticalTitle.data());
        Append(Buffer, Length, "\n");
        Append(Buffer, Length, CriticalMessage.data());
        Append(Buffer, Length, "\n");

        OutputDebugStringA(Buffer.data());

        CreateDirectoryW(L"Log", nullptr);

        const HANDLE File{CreateFileW(L"Log/critical.log", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};

        if (File != INVALID_HANDLE_VALUE) {
            DWORD Written{};
            WriteFile(File, Buffer.data(), static_cast<DWORD>(Length), &Written, nullptr);
            FlushFileBuffers(File);
            CloseHandle(File);
        }

        std::quick_exit(EXIT_FAILURE);
    }

    [[noreturn]] void HandleUnexpectedTermination() noexcept {
        ErrorHandler::Fatal("Unhandled exception", "An unhandled exception or invalid termination state prevents the program from continuing.");
    }

    [[noreturn]] void HandleAllocationFailure() {
        ErrorHandler::Fatal("Memory allocation", "The system could not allocate the requested memory.");
    }

    void EnsureLogFileOpen() {
        if (LogFile.is_open()) {
            return;
        }

        std::error_code Error{};
        std::filesystem::create_directories("Log", Error);

        if (Error) {
            return;
        }

        const auto Now{std::chrono::system_clock::now()};
        LogFile.open(std::format("Log/log_{:%Y%m%d_%H%M%S}_{}.txt", Now, GetCurrentProcessId()), std::ios::out | std::ios::app);
    }

    std::string GetDetailedStackTrace() {
        std::array<void*, 32> Frames{};
        const USHORT Count{CaptureStackBackTrace(2, static_cast<DWORD>(Frames.size()), Frames.data(), nullptr)};
        std::string Trace{"\n=== STACK TRACE ===\n"};

        for (USHORT Index{}; Index < Count; ++Index) {
            Trace += std::format("{}: 0x{:016X}\n", Index, reinterpret_cast<std::uintptr_t>(Frames[Index]));
        }

        return Trace;
    }

    std::string GetHRESULTMessage(HRESULT Result) {
        wchar_t* Buffer{};
        constexpr DWORD Flags{FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS};
        DWORD Length{FormatMessageW(Flags, nullptr, static_cast<DWORD>(Result), 0, reinterpret_cast<wchar_t*>(&Buffer), 0, nullptr)};

        if (Length == 0 && HRESULT_FACILITY(Result) == FACILITY_WIN32) {
            Length = FormatMessageW(Flags, nullptr, HRESULT_CODE(Result), 0, reinterpret_cast<wchar_t*>(&Buffer), 0, nullptr);
        }

        if (Length == 0 || Buffer == nullptr) {
            return {};
        }

        const int Size{WideCharToMultiByte(CP_UTF8, 0, Buffer, static_cast<int>(Length), nullptr, 0, nullptr, nullptr)};
        std::string Message{};

        if (Size > 0) {
            Message.resize(static_cast<std::size_t>(Size));
            WideCharToMultiByte(CP_UTF8, 0, Buffer, static_cast<int>(Length), Message.data(), Size, nullptr, nullptr);
        }

        LocalFree(Buffer);

        while (!Message.empty() && (Message.back() == '\r' || Message.back() == '\n' || Message.back() == ' ')) {
            Message.pop_back();
        }

        return Message;
    }
}

void ErrorHandler::Initialize() {
    std::set_terminate(HandleUnexpectedTermination);
    std::set_new_handler(HandleAllocationFailure);
}

void ErrorHandler::Report(std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
    const bool FatalError{Level != EErrorLevel::Warning};

    if (FatalError) {
        if (ReportingCritical) {
            ReportCriticalFallback();
        }

        ReportingCritical = true;

        const std::size_t TitleLength{std::min(Title.size(), CriticalTitle.size() - 1)};
        const std::size_t MessageLength{std::min(Message.size(), CriticalMessage.size() - 1)};

        std::copy_n(Title.begin(), TitleLength, CriticalTitle.begin());
        std::copy_n(Message.begin(), MessageLength, CriticalMessage.begin());
        CriticalTitle[TitleLength] = '\0';
        CriticalMessage[MessageLength] = '\0';
    }

    EnsureLogFileOpen();
    const std::string_view LevelName{Level == EErrorLevel::Warning ? "WARNING" : Level == EErrorLevel::Error ? "ERROR" : "CRITICAL"};
    std::string Content{std::format("[{:%Y-%m-%d %H:%M:%S} UTC] [{}] {}\n{}\n{}:{}\n{}\n", std::chrono::system_clock::now(), LevelName, Title, Message, Location.file_name(), Location.line(), Location.function_name())};

    if (FatalError) {
        Content += GetDetailedStackTrace();
    }

    OutputDebugStringA(Content.c_str());

    if (LogFile.is_open()) {
        LogFile << Content;
        LogFile.flush();
    }

    if (FatalError) {
        std::quick_exit(EXIT_FAILURE);
    }
}

void ErrorHandler::Report(bool Condition, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
    if (Condition) {
        Report(Title, Message, Level, Location);
    }
}

void ErrorHandler::ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
    if (FAILED(Result)) {
        Report(Title, std::format("{}\nHRESULT: 0x{:08X}\nFacility: {}\nCode: {}\nSystem: {}", Message, static_cast<std::uint32_t>(Result), HRESULT_FACILITY(Result), HRESULT_CODE(Result), GetHRESULTMessage(Result)), Level, Location);
    }
}

void ErrorHandler::ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location) {
    if (ErrorCode != ERROR_SUCCESS) {
        ReportHRESULT(HRESULT_FROM_WIN32(ErrorCode), Title, Message, Level, Location);
    }
}

[[noreturn]] void ErrorHandler::Fatal(std::string_view Title, std::string_view Message, const std::source_location& Location) {
    if (ReportingCritical) {
        ReportCriticalFallback();
    }

    Report(Title, Message, EErrorLevel::Critical, Location);
    std::quick_exit(EXIT_FAILURE);
}
