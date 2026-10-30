/**
 * @file Main.cpp
 * @brief 게임 런타임의 Windows 실행 진입점입니다.
 * @ingroup Game
 */

#include "FGameApplication.h"
#include "Core/Base/ErrorHandler.h"

#include <shellapi.h>
#include <string_view>

/**
 * @brief 명령행을 해석하고 게임 애플리케이션을 실행합니다.
 * @details 기본 프로젝트는 실행 파일 옆의 Game.mproject이며, `--project <경로>`로 다른 프로젝트를 지정할 수 있습니다.
 * @return 정상 종료하면 0, COM 초기화·명령행 해석·게임 실행에 실패하면 1입니다.
 */
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Core::ErrorHandler::Initialize();

    const HRESULT ComResult{CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)};

    if (FAILED(ComResult)) {
        return 1;
    }

    std::array<wchar_t, 32768> Executable{};
    const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};

    if (Length == 0 || Length >= Executable.size()) {
        CoUninitialize();
        return 1;
    }

    std::filesystem::path ProjectFile{std::filesystem::path{Executable.data()}.parent_path() / "Game.mproject"};
    bool ValidArguments{true};
    int ArgumentCount{};
    wchar_t** Arguments{CommandLineToArgvW(GetCommandLineW(), &ArgumentCount)};

    if (Arguments == nullptr) {
        CoUninitialize();
        return 1;
    }

    for (int Index{1}; Index < ArgumentCount; ++Index) {
        const std::wstring_view Argument{Arguments[Index]};

        if (Argument == L"--project" && Index + 1 < ArgumentCount) {
            ProjectFile = Arguments[++Index];
        } else {
            ValidArguments = false;
        }
    }

    LocalFree(Arguments);

    int Result{1};

    if (ValidArguments) {
        Game::FGameApplication Application{};
        Result = Application.Run(ProjectFile);
    } else {
        Core::ErrorHandler::Report("Game", "Usage: MacawGame.exe [--project Project.mproject]", Core::ErrorHandler::EErrorLevel::Error);
    }

    CoUninitialize();

    return Result;
}
