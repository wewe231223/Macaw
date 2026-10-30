/**
 * @file ErrorHandler.h
 * @brief 로그, 시스템 오류 문자열과 치명적 오류 종료를 제공합니다.
 * @ingroup Core
 */

#pragma once

#include <Windows.h>
#include <source_location>
#include <string_view>

namespace Core {

    /**
     * @brief 로그 기록과 심각도별 오류 종료를 제공합니다.
     * @ingroup Core
     */
    namespace ErrorHandler {
        /**
         * @brief 로그 보고의 심각도와 중단 여부를 구분합니다.
         * @ingroup Core
         */
        enum class EErrorLevel {
            /**
             * @brief 경고를 기록하고 실행을 계속합니다.
             */
            Warning,
            /**
             * @brief 오류를 기록하고 실행을 계속합니다.
             */
            Error,
            /**
             * @brief 치명적 오류를 기록한 뒤 실행을 종료합니다.
             */
            Critical
        };

        /**
         * @brief 프로세스 예외 종료와 메모리 할당 실패 처리기를 설치합니다.
         */
        void Initialize();

        /**
         * @brief 심각도와 호출 위치를 포함하여 오류를 기록하고 Critical이면 프로그램을 중단합니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] Level 보고할 오류의 심각도입니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         */
        void Report(std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location = std::source_location::current());

        /**
         * @brief 심각도와 호출 위치를 포함하여 오류를 기록하고 Critical이면 프로그램을 중단합니다.
         * @param[in] Condition 참일 때만 보고합니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] Level 보고할 오류의 심각도입니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         */
        void Report(bool Condition, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location = std::source_location::current());

        /**
         * @brief 실패한 HRESULT에 시스템 오류 문자열을 붙여 보고합니다.
         * @param[in] Result 실패 여부를 검사할 HRESULT입니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] Level 보고할 오류의 심각도입니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         */
        void ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location = std::source_location::current());

        /**
         * @brief 0이 아닌 Win32 오류 코드에 시스템 오류 문자열을 붙여 보고합니다.
         * @param[in] ErrorCode 실패 여부를 검사할 Win32 오류 코드입니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] Level 보고할 오류의 심각도입니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         */
        void ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, Core::ErrorHandler::EErrorLevel Level, const std::source_location& Location = std::source_location::current());

        /**
         * @brief 치명적 오류를 기록한 뒤 프로그램을 종료하며 호출자로 돌아오지 않습니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] Location 오류가 발생한 원래 호출 위치입니다.
         */
        [[noreturn]] void Fatal(std::string_view Title, std::string_view Message, const std::source_location& Location = std::source_location::current());
    }

}
