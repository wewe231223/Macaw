/**
 * @file ErrorHandler.cpp
 * @brief 로그, 시스템 오류 문자열과 치명적 오류 종료를 제공합니다.
 * @ingroup Core
 */

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

namespace Core {

    /**
     * @brief 이 구현 파일에서만 사용하는 내부 상태와 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 실행 중인 프로세스의 로그 출력 스트림입니다.
         */
        std::ofstream LogFile{};

        /**
         * @brief 현재 스레드가 치명적 오류를 보고 중인지 여부입니다.
         */
        thread_local bool ReportingCritical{};

        /**
         * @brief 치명적 오류 재진입 대비 고정 크기 제목 버퍼입니다.
         */
        thread_local std::array<char, 257> CriticalTitle{};

        /**
         * @brief 치명적 오류 재진입 대비 고정 크기 메시지 버퍼입니다.
         */
        thread_local std::array<char, 3501> CriticalMessage{};

        /**
         * @brief 고정 버퍼의 남은 공간에 문자열을 붙이고 널 종료를 유지합니다.
         * @param[in] Buffer 널 종료를 유지하며 기록할 출력 버퍼입니다.
         * @param[in,out] Length 현재 사용한 길이이며 추가한 문자 수만큼 증가합니다.
         * @param[in] Text 파싱하거나 버퍼에 복사할 문자열입니다.
         */
        void Append(std::span<char> Buffer, std::size_t& Length, std::string_view Text) {
            const std::size_t Count{std::min(Text.size(), Buffer.size() - Length - 1)};
            std::copy_n(Text.begin(), Count, Buffer.begin() + Length);
            Length += Count;
            Buffer[Length] = '\0';
        }

        /**
         * @brief 오류 보고가 재진입한 경우 고정 버퍼로 최소 로그를 쓰고 즉시 종료합니다.
         */
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

        /**
         * @brief 처리되지 않은 예외의 종료를 치명적 오류 보고로 연결합니다.
         */
        [[noreturn]] void HandleUnexpectedTermination() noexcept {
            Core::ErrorHandler::Fatal("Unhandled exception", "An unhandled exception or invalid termination state prevents the program from continuing.");
        }

        /**
         * @brief 메모리 할당 실패를 치명적 오류 보고로 연결합니다.
         */
        [[noreturn]] void HandleAllocationFailure() {
            Core::ErrorHandler::Fatal("Memory allocation", "The system could not allocate the requested memory.");
        }

        /**
         * @brief 로그 디렉터리와 프로세스별 로그 파일을 필요할 때 엽니다.
         */
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

        /**
         * @brief 현재 호출 스택의 주소를 사람이 읽을 수 있는 문자열로 구성합니다.
         * @return 현재 호출 스택의 주소를 사람이 읽을 수 있는 문자열로 구성합니다.입니다.
         */
        std::string GetDetailedStackTrace() {
            std::array<void*, 32> Frames{};
            const USHORT Count{CaptureStackBackTrace(2, static_cast<DWORD>(Frames.size()), Frames.data(), nullptr)};
            std::string Trace{"\n=== STACK TRACE ===\n"};

            for (USHORT Index{}; Index < Count; ++Index) {
                Trace += std::format("{}: 0x{:016X}\n", Index, reinterpret_cast<std::uintptr_t>(Frames[Index]));
            }

            return Trace;
        }

        /**
         * @brief HRESULT에 대응하는 Windows 시스템 메시지를 문자열로 변환합니다.
         * @param[in] Result 실패 여부를 검사할 HRESULT입니다.
         * @return HRESULT에 대응하는 Windows 시스템 메시지를 문자열로 변환합니다.입니다.
         */
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

    /**
     * @fn void Core::ErrorHandler::Initialize()
     * @brief 프로세스 예외 종료와 메모리 할당 실패 처리기를 설치합니다.
     */
    void Core::ErrorHandler::Initialize() {
        std::set_terminate(HandleUnexpectedTermination);
        std::set_new_handler(HandleAllocationFailure);
    }

    /**
     * @fn void Core::ErrorHandler::Report(std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location)
     * @brief 심각도와 호출 위치를 포함하여 오류를 기록하고 Critical이면 프로그램을 중단합니다.
     */
    void Core::ErrorHandler::Report(std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location) {
        const bool FatalError{Level != Core::ErrorHandler::EErrorLevel::Warning};

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

        const std::string_view LevelName{Level == Core::ErrorHandler::EErrorLevel::Warning ? "WARNING" : Level == Core::ErrorHandler::EErrorLevel::Error ? "ERROR" : "CRITICAL"};
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

    /**
     * @fn void Core::ErrorHandler::Report(bool Condition, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location)
     * @brief 심각도와 호출 위치를 포함하여 오류를 기록하고 Critical이면 프로그램을 중단합니다.
     */
    void Core::ErrorHandler::Report(bool Condition, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location) {
        if (Condition) {
            Core::ErrorHandler::Report(Title, Message, Level, Location);
        }
    }

    /**
     * @fn void Core::ErrorHandler::ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location)
     * @brief 실패한 HRESULT에 시스템 오류 문자열을 붙여 보고합니다.
     */
    void Core::ErrorHandler::ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location) {
        if (FAILED(Result)) {
            Core::ErrorHandler::Report(Title, std::format("{}\nHRESULT: 0x{:08X}\nFacility: {}\nCode: {}\nSystem: {}", Message, static_cast<std::uint32_t>(Result), HRESULT_FACILITY(Result), HRESULT_CODE(Result), GetHRESULTMessage(Result)), Level, Location);
        }
    }

    /**
     * @fn void Core::ErrorHandler::ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location)
     * @brief 0이 아닌 Win32 오류 코드에 시스템 오류 문자열을 붙여 보고합니다.
     */
    void Core::ErrorHandler::ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location) {
        if (ErrorCode != ERROR_SUCCESS) {
            Core::ErrorHandler::ReportHRESULT(HRESULT_FROM_WIN32(ErrorCode), Title, Message, Level, Location);
        }
    }

    /**
     * @fn [[noreturn]] void Core::ErrorHandler::Fatal(std::string_view Title, std::string_view Message, const std::source_location& Location)
     * @brief 치명적 오류를 기록한 뒤 프로그램을 종료하며 호출자로 돌아오지 않습니다.
     */
    [[noreturn]] void Core::ErrorHandler::Fatal(std::string_view Title, std::string_view Message, const std::source_location& Location) {
        Core::ErrorHandler::Report(Title, Message, Core::ErrorHandler::EErrorLevel::Critical, Location);
    }

}
