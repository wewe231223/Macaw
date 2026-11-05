/**
 * @file FScriptRuntime.h
 * @brief 프로세스에 CoreCLR을 호스팅하고 C# 스크립트 어셈블리를 관리하는 런타임의 선언입니다.
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <filesystem>
#include <functional>
#include <memory>

namespace Script {

    /**
     * @brief 실행 파일 옆에 배치된 .NET 런타임을 기동하고 C# 스크립트의 로드와 언로드를 담당합니다.
     * @note CoreCLR은 프로세스당 한 번만 기동할 수 있고 종료 후 다시 기동할 수 없으므로 단일 인스턴스로 관리합니다.
     */
    class FScriptRuntime final {
    public:
        /**
         * @brief C# 로그 메시지를 전달받는 함수 타입입니다.
         */
        using FLogFunction = std::function<void(const Core::FString&)>;

    private:
        /**
         * @brief 프로세스 수명 동안 유지되는 런타임 상태를 구성합니다.
         */
        FScriptRuntime();

        /**
         * @brief 런타임 상태를 정리합니다.
         */
        ~FScriptRuntime();

        /**
         * @brief 소유 상태의 중복을 막기 위해 복사를 금지합니다.
         */
        FScriptRuntime(const Script::FScriptRuntime&) = delete;

        /**
         * @brief 소유 상태의 중복을 막기 위해 복사를 금지합니다.
         */
        Script::FScriptRuntime& operator=(const Script::FScriptRuntime&) = delete;

    public:
        /**
         * @brief 프로세스에 하나뿐인 런타임을 반환합니다.
         * @return 런타임의 비소유 주소입니다.
         */
        static Script::FScriptRuntime* Get();

        /**
         * @brief C# 로그 메시지를 받을 콜백을 설정합니다.
         * @param[in] Function 메시지를 받을 콜백입니다.
         */
        void SetLogFunction(FLogFunction Function);

        /**
         * @brief 실행 파일 옆의 dotnet 폴더에서 CoreCLR을 기동하고 Macaw.Engine을 로드합니다.
         * @param[out] Error 실패 원인입니다.
         * @return 이미 기동되었거나 기동에 성공하면 true, 실패하면 false입니다.
         */
        bool Initialize(Core::FString& Error);

        /**
         * @brief CoreCLR이 기동되어 있는지 반환합니다.
         * @return 기동되어 있으면 true입니다.
         */
        bool IsInitialized() const;

        /**
         * @brief 현재 스크립트를 언로드하고 지정한 어셈블리를 새로 로드합니다.
         * @param[in] Assembly 로드할 스크립트 어셈블리의 경로입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 로드에 성공하면 true, 실패하면 false입니다.
         * @note 이전 스크립트가 수집되지 않아도 경고만 남기고 새 어셈블리를 로드합니다.
         */
        bool LoadScripts(const std::filesystem::path& Assembly, Core::FString& Error);

        /**
         * @brief 현재 스크립트를 언로드합니다.
         * @return 스크립트가 없거나 수집에 성공하면 true, 무언가 붙잡고 있으면 false입니다.
         */
        bool UnloadScripts();

        /**
         * @brief 스크립트 어셈블리가 로드되어 있는지 반환합니다.
         * @return 로드되어 있으면 true입니다.
         */
        bool HasScripts() const;

        /**
         * @brief 스크립트 클래스의 인스턴스를 만들어 한 번 Tick한 뒤 파괴합니다.
         * @param[in] ClassName 네임스페이스를 포함한 클래스 이름입니다.
         * @param[in] DeltaTime 전달할 시간 간격입니다.
         * @param[out] Error 실패 원인입니다.
         * @return 인스턴스를 만들어 실행했으면 true, 실패하면 false입니다.
         * @note 호스팅 검증용입니다. 스크립트 안에서 발생한 예외는 로그로만 보고되며 실패로 처리하지 않습니다.
         */
        bool RunOnce(const Core::FString& ClassName, float DeltaTime, Core::FString& Error);

        /**
         * @brief 등록된 로그 콜백에 메시지를 전달합니다.
         * @param[in] Message 전달할 메시지입니다.
         */
        void WriteLog(const Core::FString& Message) const;

    private:
        /**
         * @brief hostfxr와 C# 사이의 함수 테이블을 담는 내부 상태입니다.
         */
        struct FState;

        /**
         * @brief 기동에 성공한 뒤 유효한 내부 상태입니다.
         */
        std::unique_ptr<FState> mState{};

        /**
         * @brief C# 메시지를 받는 선택적 콜백입니다.
         */
        FLogFunction mLogFunction{};

        /**
         * @brief 스크립트 어셈블리가 로드되어 있는지 나타냅니다.
         */
        bool mHasScripts{};
    };

}
