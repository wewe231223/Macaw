/**
 * @file Main.cpp
 * @brief Windows 실행 진입점에서 애플리케이션을 시작합니다.
 * @ingroup App
 */

#include "Main.h"
#include "Core/Base/ErrorHandler.h"

/**
 * @brief Windows 실행 진입점에서 오류 처리를 초기화하고 애플리케이션을 실행합니다.
 * @param[in] Instance Windows가 전달한 현재 실행 인스턴스입니다.
 * @param[in] Previous Win32 호환 인수로 사용하지 않습니다.
 * @param[in] CommandLine Windows가 전달한 명령행 문자열입니다.
 * @param[in] ShowCommand Windows가 전달한 초기 창 표시 옵션입니다.
 * @return 정상 종료 시 0입니다.
 */
int WINAPI WinMain(HINSTANCE Instance, HINSTANCE Previous, LPSTR CommandLine, int ShowCommand) {
    static_cast<void>(Instance);
    static_cast<void>(Previous);
    static_cast<void>(CommandLine);
    static_cast<void>(ShowCommand);

    ErrorHandler::Initialize();

    FApplication Application{};

    return Application.Run();
}
