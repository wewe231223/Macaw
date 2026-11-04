/**
 * @file LuaVM.h
 * @brief Lua 상태의 수명, 엔진 API 바인딩과 스크립트 실행에 대한 선언입니다.
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

#include <functional>
#include <filesystem>
#include <sol/sol.hpp>

namespace Core {
    class FInput;
}

namespace Script {

    /**
     * @brief Lua 상태의 수명, 엔진 API 바인딩과 스크립트 실행을 담당합니다.
     */
    class FLuaVM final {
    public:
        /**
         * @brief Lua 로그 메시지를 전달받는 함수 타입입니다.
         */
        using FLogFunction = std::function<void(const Core::FString&)>;

    public:
        /**
         * @brief FLuaVM의 초기 상태를 구성합니다.
         */
        FLuaVM() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~FLuaVM();

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        FLuaVM(const Script::FLuaVM&) = delete;

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        Script::FLuaVM& operator=(const Script::FLuaVM&) = delete;

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        FLuaVM(Script::FLuaVM&&) = delete;

        /**
         * @brief 소유 상태와 의존성의 중복을 막기 위해 복사 또는 이동을 금지합니다.
         */
        Script::FLuaVM& operator=(Script::FLuaVM&&) = delete;

    public:
        /**
         * @brief Lua 상태와 표준 라이브러리를 준비하고 엔진 및 로그 API를 등록합니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool Initialize();

        /**
         * @brief Lua 상태를 해제하여 VM을 종료합니다.
         */
        void Shutdown();

        /**
         * @brief Lua 소스 문자열을 보호된 호출로 실행하고 실패를 로그에 기록합니다.
         * @param[in] Source 실행할 Lua 소스 문자열입니다.
         * @param[in] ChunkName Lua 오류 위치에 표시할 소스 청크의 이름입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ExecuteString(const Core::FString& Source, const Core::FString& ChunkName = "@StringChunk");

        /**
         * @brief 초기화된 Lua 상태에서 스크립트 파일을 실행하고 실패를 로그에 기록합니다.
         * @param[in] ScriptPath 실행할 Lua 스크립트 파일 경로입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool ExecuteFile(const std::filesystem::path& ScriptPath);
        bool Tick(float DeltaTime);

        /**
         * @brief 사용 가능한 Lua 상태가 생성되어 있는지 반환합니다.
         * @return 검사 조건을 만족하면 true, 그렇지 않으면 false입니다.
         */
        bool IsInitialized() const;

        /**
         * @brief Lua 상태의 비소유 주소를 반환하며 초기화 전에는 nullptr을 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        sol::state* GetState();

        /**
         * @brief Lua 상태의 비소유 주소를 반환하며 초기화 전에는 nullptr을 반환합니다.
         * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며, 사용할 수 없으면 nullptr입니다.
         */
        const sol::state* GetState() const;

        /**
         * @brief Lua 실행 진단과 print 출력을 받을 콜백을 설정합니다.
         * @param[in] Function 스크립트 출력과 진단을 받을 로그 콜백입니다.
         */
        void SetLogFunction(FLogFunction Function);
        void SetInput(const Core::FInput* Input);

    private:
        /**
         * @brief 벡터·회전·액터 등 엔진 타입과 함수를 Lua에 등록합니다.
         */
        void BindEngineAPI();
        void BindInputAPI();

        /**
         * @brief Lua print 출력을 엔진 로그 콜백으로 전달하도록 바인딩합니다.
         */
        void BindLoggingAPI();

        /**
         * @brief 보호된 Lua 호출 결과를 검사하고 오류에 실행 문맥을 붙여 기록합니다.
         * @param[in] Result 유효성과 오류를 검사할 보호된 Lua 호출 결과입니다.
         * @param[in] Context 로그에 표시할 실행 문맥입니다.
         * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
         */
        bool CheckLuaResult(sol::protected_function_result& Result, const Core::FString& Context) const;

        /**
         * @brief 등록된 로그 콜백 또는 디버그 출력에 메시지를 전달합니다.
         * @param[in] Message 로그에 전달할 메시지입니다.
         */
        void WriteLog(const Core::FString& Message) const;

    private:
        /**
         * @brief 이 VM이 단독 소유하는 Lua 상태입니다.
         */
        std::unique_ptr<sol::state> mState{};

        /**
         * @brief 스크립트 출력과 실행 진단을 받는 선택적 로그 콜백입니다.
         */
        FLogFunction mLogFunction{};
        const Core::FInput* mInput{};
    };

}
