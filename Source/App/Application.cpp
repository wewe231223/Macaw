/**
 * @file Application.cpp
 * @brief 플랫폼, 렌더러와 에디터를 연결하는 애플리케이션 수명을 관리합니다.
 * @ingroup App
 */

#include "Application.h"
#include "AppReflection.h"
#include "CoreUObject/UObject.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/Memory.h"

/**
 * @brief FApplication의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FApplication::FApplication() {
    ReflectionModules::RegisterAppTypes();
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FApplication::~FApplication() {
    Shutdown();
}

/**
 * @brief 창과 렌더러·에디터를 초기화하고 종료 요청까지 프레임 루프를 실행합니다.
 */
int FApplication::Run() {
    if (!mPlatform.Create(L"Macaw", 1440, 900)) {
        ErrorHandler::Fatal("Application", "Window creation failed");
    }

    if (!mRenderer.Initialize(mPlatform.GetWindowHandle(), mPlatform.GetWidth(), mPlatform.GetHeight())) {
        ErrorHandler::Fatal("Application", "Renderer initialization failed");
    }

    if (!mEngine.InitializeEditor(&mRenderer, &mPlatform)) {
        ErrorHandler::Fatal("Application", "Editor initialization failed");
    }

    while (mPlatform.PumpMessages() && !mEngine.WantsExit()) {
        if (mPlatform.IsMinimized()) {
            MsgWaitForMultipleObjects(0, nullptr, false, 16, QS_ALLINPUT);
            continue;
        }

        mEngine.TickFrame(mTimer.Tick());
        mRenderer.Present();
    }

    mEngine.Shutdown();
    ErrorHandler::Report(FMemory::GetStats().mHeapBytes != 0, "Application", "UObject allocations remain after editor shutdown", ErrorHandler::EErrorLevel::Critical);
    Shutdown();

    return 0;
}

/**
 * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
 */
void FApplication::Shutdown() {
    mEngine.Shutdown();
    mRenderer.Shutdown();
    mPlatform.Shutdown();
}
