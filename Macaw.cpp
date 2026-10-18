#include "pch.h"
#include "Core/Base/ErrorHandler.h"

#include "Platform/Windows/FWindowsGraphicsSettings.h"
#include "Application/FApplication.h"

int APIENTRY wWinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ LPWSTR CommandLine, _In_ int ShowCommand) {
    ErrorHandler::Initialize();
    FWindowsGraphicsSettings::Configure();

    UNREFERENCED_PARAMETER(PreviousInstance);
    UNREFERENCED_PARAMETER(CommandLine);

    FApplication Application{};

    return Application.Run(Instance, ShowCommand);
}
