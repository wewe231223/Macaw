/**
 * @file ErrorHandler.h
 * @brief 오류 수준별 보고, 종료 처리기 등록 및 Windows 오류 코드 보고 인터페이스를 정의한다.
 */
#pragma once

#include <source_location>
#include <string_view>

#ifdef _WIN32
#include <Windows.h>
#endif

/** @brief 오류를 로그로 기록하고 오류 수준에 따라 사용자 알림과 프로세스 종료를 처리한다. */
namespace ErrorHandler {
    /** @brief 오류 보고 시 수행할 알림 및 종료 수준을 지정한다. */
    enum class EErrorLevel {
        /** @brief 경고를 로그에 기록하고 실행을 계속한다. */
        Warning,
        /** @brief 오류를 로그에 기록하고 Windows에서는 메시지 상자를 표시한 뒤 실행을 계속한다. */
        Error,
        /** @brief 치명적 오류에 스택 추적 정보를 추가하고 Windows에서는 메시지 상자를 표시한 뒤 실패 코드로 프로세스를 종료한다. */
        Critical
    };

    /**
     * @brief 예기치 않은 종료와 C++ 동적 메모리 할당 실패를 처리할 전역 처리기를 등록한다.
     * @details std::set_terminate()와 std::set_new_handler()로 처리기를 등록한다. 두 처리기는 치명적 오류를 보고하고 프로세스를 종료한다.
     */
    void Initialize();

    /**
     * @brief 오류를 보고하고 지정한 수준에 맞는 후속 처리를 수행한다.
     * @param Title 오류 제목. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Message 오류 설명. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Level 로그 기록, 메시지 상자 표시 및 프로세스 종료를 결정할 오류 수준.
     * @details 최초 보고 시 작업 디렉터리의 Log 폴더에 타임스탬프 기반 로그 파일을 연다. 파일을 열 수 있으면 오류를 기록하고 버퍼를 비운다.
     * @details Windows에서는 Error와 Critical 수준에 메시지 상자를 표시한다. Critical 수준에는 스택 추적 정보를 추가하며, 상세 추적은 ENABLE_ASSERTION_STACKTRACE 설정에 따라 제공된다.
     * @note Critical 수준은 std::_Exit(EXIT_FAILURE)로 프로세스를 종료하므로 반환하지 않으며 자동 객체의 소멸자도 호출하지 않는다.
     */
    void Report(std::string_view Title, std::string_view Message, EErrorLevel Level);
    /**
     * @brief 조건이 참일 때만 오류를 보고한다.
     * @param BCondition true이면 오류를 보고하고, false이면 아무 처리 없이 반환한다.
     * @param Title 오류 제목. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Message 오류 설명. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Level 오류를 보고할 때 적용할 오류 수준.
     * @details 조건이 참이면 조건 인수가 없는 Report()에 처리를 위임한다. Critical 수준의 보고는 프로세스를 종료한다.
     */
    void Report(bool BCondition, std::string_view Title, std::string_view Message, EErrorLevel Level);

#ifdef _WIN32
    /**
     * @brief 실패한 HRESULT에 시스템 오류 정보와 호출 위치를 추가하여 보고한다.
     * @param Result 검사할 HRESULT. SUCCEEDED(Result)가 참이면 아무 처리 없이 반환한다.
     * @param Title 오류 제목. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Message 시스템 오류 정보 앞에 추가할 오류 설명.
     * @param Level 실패한 결과를 보고할 때 적용할 오류 수준.
     * @param Location 오류 발생 위치. 생략하면 호출 지점의 파일, 줄 번호 및 함수 정보를 사용한다.
     * @details 실패 시 HRESULT의 16진수 값, Facility 값, 오류 코드, 시스템 메시지 및 호출 위치를 설명에 추가한 뒤 Report()로 보고한다.
     * @note Windows에서만 제공된다. 실패한 결과를 Critical 수준으로 보고하면 프로세스를 종료한다.
     */
    void ReportHRESULT(HRESULT Result, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());
    /**
     * @brief Win32 오류 코드를 HRESULT로 변환하여 시스템 오류 정보와 호출 위치를 보고한다.
     * @param ErrorCode 검사할 Win32 오류 코드. ERROR_SUCCESS이면 아무 처리 없이 반환한다.
     * @param Title 오류 제목. Windows 메시지 상자에서는 UTF-8 문자열로 해석한다.
     * @param Message 시스템 오류 정보 앞에 추가할 오류 설명.
     * @param Level 오류 코드를 보고할 때 적용할 오류 수준.
     * @param Location 오류 발생 위치. 생략하면 호출 지점의 파일, 줄 번호 및 함수 정보를 사용한다.
     * @details HRESULT_FROM_WIN32()로 변환한 결과를 ReportHRESULT()에 전달한다.
     * @note Windows에서만 제공된다. Critical 수준의 오류 보고는 프로세스를 종료한다.
     */
    void ReportWin32(DWORD ErrorCode, std::string_view Title, std::string_view Message, EErrorLevel Level, const std::source_location& Location = std::source_location::current());
#endif
}
