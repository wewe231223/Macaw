#include "Main.h"
#include "Core/Base/ErrorHandler.h"

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE Previous, LPSTR CommandLine, int ShowCommand) {
    static_cast<void>(Instance);
    static_cast<void>(Previous);
    static_cast<void>(CommandLine);
    static_cast<void>(ShowCommand);

    ErrorHandler::Initialize();

    FApplication Application{};

    return Application.Run();
}
