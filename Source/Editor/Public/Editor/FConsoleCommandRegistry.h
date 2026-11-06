/**
 * @file FConsoleCommandRegistry.h
 * @brief 고정·동적 콘솔 명령의 실행·도움말·자동 완성을 제공합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/Types.h"

#include <functional>
#include <string_view>

namespace Editor {

    /**
     * @brief 실행할 콘솔 명령의 이름·사용법과 입력·인자 문자열 뷰입니다.
     */
    struct FConsoleInvocation {
        /**
         * @brief 콘솔 명령 또는 통계 항목의 이름입니다.
         */
        std::string_view mName{};

        /**
         * @brief 콘솔 도움말에 표시할 사용법입니다.
         */
        std::string_view mUsage{};

        /**
         * @brief 명령을 실행할 때 전달한 원본 콘솔 입력의 문자열 뷰입니다.
         */
        std::string_view mInput{};

        /**
         * @brief 명령 이름 뒤에서 분리한 인자 문자열 뷰입니다.
         */
        std::string_view mArguments{};
    };

    /**
     * @brief 명령 이름·사용법·인자 후보와 실행 콜백입니다.
     */
    struct FConsoleCommand {
        /**
         * @brief 콘솔 명령 또는 통계 항목의 이름입니다.
         */
        Core::FString mName{};

        /**
         * @brief 콘솔 도움말에 표시할 사용법입니다.
         */
        Core::FString mUsage{};

        /**
         * @brief 자동 완성에 사용할 명령 인자 후보입니다.
         */
        Core::TArray<Core::FString> mArguments{};

        /**
         * @brief 명령이 일치하면 호출할 콜백입니다.
         */
        std::function<void(const Editor::FConsoleInvocation&)> mExecute{};
    };

    /**
     * @brief 고정·동적 콘솔 명령의 실행·도움말·자동 완성을 제공합니다.
     */
    class FConsoleCommandRegistry {
    public:
        /**
         * @brief 조회 시점의 콘솔 명령 목록을 제공하는 콜백 타입입니다.
         */
        using FCommandSource = std::function<Core::TArray<Editor::FConsoleCommand>()>;

    public:
        /**
         * @brief 콘솔 명령을 고정 명령 목록의 끝에 추가합니다.
         * @param[in] Command 등록할 이름·사용법·인자 후보·실행 콜백입니다.
         */
        void Register(Editor::FConsoleCommand Command);

        /**
         * @brief 조회 시점에 콘솔 명령을 제공할 동적 공급자를 등록합니다.
         * @param[in] Source 조회 시점에 명령 목록을 반환할 공급자입니다.
         */
        void RegisterSource(Editor::FConsoleCommandRegistry::FCommandSource Source);

        /**
         * @brief 입력에 일치하는 콘솔 명령을 찾아 분리한 인자를 전달하고 실행합니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         * @return 명령을 찾아 실행 콜백을 호출했으면 true입니다.
         */
        bool Execute(std::string_view Input) const;

        /**
         * @brief 등록된 고정·동적 콘솔 명령의 이름과 사용법을 수집합니다.
         * @return 명령 이름·사용법 목록입니다.
         */
        Core::TArray<Core::FString> GetHelp() const;

        /**
         * @brief 명령 이름과 인자 후보에서 입력에 맞는 자동 완성 목록을 만듭니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         * @return 입력 접두어에 일치하는 후보 목록입니다.
         */
        Core::TArray<Core::FString> GetSuggestions(std::string_view Input) const;

    private:
        /**
         * @brief 고정 명령과 동적 공급자가 반환한 명령을 순회합니다.
         * @param[in] Visit 각 콘솔 명령에 적용할 콜백입니다.
         */
        void VisitCommands(const std::function<void(const Editor::FConsoleCommand&)>& Visit) const;

    private:
        /**
         * @brief 직접 등록한 고정 콘솔 명령 목록입니다.
         */
        Core::TArray<Editor::FConsoleCommand> mCommands{};

        /**
         * @brief 동적 콘솔 명령을 제공하는 콜백 목록입니다.
         */
        Core::TArray<Editor::FConsoleCommandRegistry::FCommandSource> mSources{};
    };

}
