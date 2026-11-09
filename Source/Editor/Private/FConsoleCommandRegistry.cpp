/**
 * @file FConsoleCommandRegistry.cpp
 * @brief 고정·동적 콘솔 명령의 실행·도움말·자동 완성을 제공합니다.
 * @ingroup Editor
 */

#include "Editor/FConsoleCommandRegistry.h"

#include <cctype>
#include <optional>
#include <utility>

namespace Editor {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 콘솔 구분자로 사용하는 공백 문자인지 검사합니다.
         * @param[in] Character 검사할 문자입니다.
         * @return 콘솔 구분자 공백이면 true입니다.
         */
        bool IsConsoleSpace(unsigned char Character) {
            return std::isspace(Character) != 0;
        }

        /**
         * @brief 문자열 뷰의 시작을 콘솔 공백이 끝난 뒤로 옮깁니다.
         * @param[in] Text 해석·변환·탐색할 원문 문자열입니다.
         */
        void SkipConsoleSpaces(std::string_view& Text) {
            while (!Text.empty() && IsConsoleSpace(Text.front())) {
                Text.remove_prefix(1);
            }
        }

        /**
         * @brief 명령 이름과 단어 경계가 일치하는지 검사하고 후속 인자를 분리합니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         * @param[in] Name 조회·등록·출력할 대상 이름입니다.
         * @return 일치한 명령 뒤의 인자 뷰이며 불일치하면 빈 optional입니다.
         */
        std::optional<std::string_view> MatchCommand(std::string_view Input, std::string_view Name) {
            SkipConsoleSpaces(Input);

            for (const unsigned char Character : Name) {
                if (Input.empty()) {
                    return std::nullopt;
                }

                const unsigned char Next{static_cast<unsigned char>(Input.front())};

                if (Character == ' ' && IsConsoleSpace(Next)) {
                    SkipConsoleSpaces(Input);
                } else if (std::tolower(Character) == std::tolower(Next)) {
                    Input.remove_prefix(1);
                } else {
                    return std::nullopt;
                }
            }

            if (!Input.empty() && !IsConsoleSpace(Input.front())) {
                return std::nullopt;
            }

            SkipConsoleSpaces(Input);

            return Input;
        }

        /**
         * @brief 자동 완성 비교에 사용할 앞 공백과 대소문자를 정규화합니다.
         * @param[in] Input 해석할 원본 입력 문자열입니다.
         * @return 해석·변환한 문자열입니다.
         */
        Core::FString NormalizeConsolePrefix(std::string_view Input) {
            Core::FString Prefix{};

            for (const unsigned char Character : Input) {
                if (!IsConsoleSpace(Character)) {
                    Prefix += static_cast<char>(std::tolower(Character));
                } else if (!Prefix.empty() && Prefix.back() != ' ') {
                    Prefix += ' ';
                }
            }

            return Prefix;
        }
    }

    /**
     * @brief 콘솔 명령을 고정 명령 목록의 끝에 추가합니다.
     */
    void Editor::FConsoleCommandRegistry::Register(Editor::FConsoleCommand Command) {
        mCommands.push_back(std::move(Command));
    }

    /**
     * @brief 조회 시점에 콘솔 명령을 제공할 동적 공급자를 등록합니다.
     */
    void Editor::FConsoleCommandRegistry::RegisterSource(Editor::FConsoleCommandRegistry::FCommandSource Source) {
        mSources.push_back(std::move(Source));
    }

    /**
     * @brief 입력에 일치하는 콘솔 명령을 찾아 분리한 인자를 전달하고 실행합니다.
     */
    bool Editor::FConsoleCommandRegistry::Execute(std::string_view Input) const {
        std::optional<Editor::FConsoleCommand> Selected{};
        std::string_view Arguments{};

        VisitCommands([&Selected, &Arguments, Input](const Editor::FConsoleCommand& Command) {
            const std::optional<std::string_view> Match{MatchCommand(Input, Command.mName)};

            if (Match && (!Selected || Command.mName.size() > Selected->mName.size())) {
                Selected = Command;
                Arguments = *Match;
            }
        });

        if (!Selected) {
            return false;
        }

        Selected->mExecute({Selected->mName, Selected->mUsage, Input, Arguments});

        return true;
    }

    /**
     * @brief 등록된 고정·동적 콘솔 명령의 이름과 사용법을 수집합니다.
     */
    Core::TArray<Core::FString> Editor::FConsoleCommandRegistry::GetHelp() const {
        Core::TArray<Core::FString> Lines{};

        VisitCommands([&Lines](const Editor::FConsoleCommand& Command) {
            Lines.push_back(Command.mUsage.empty() ? Command.mName : Command.mName + " " + Command.mUsage);
        });

        return Lines;
    }

    /**
     * @brief 명령 이름과 인자 후보에서 입력에 맞는 자동 완성 목록을 만듭니다.
     */
    Core::TArray<Core::FString> Editor::FConsoleCommandRegistry::GetSuggestions(std::string_view Input) const {
        Core::TArray<Core::FString> Suggestions{};
        const Core::FString Prefix{NormalizeConsolePrefix(Input)};

        if (Prefix.empty()) {
            return Suggestions;
        }

        VisitCommands([&Suggestions, &Prefix](const Editor::FConsoleCommand& Command) {
            const Core::FString Name{NormalizeConsolePrefix(Command.mName)};

            if (Name.starts_with(Prefix)) {
                Suggestions.push_back(Command.mUsage.empty() ? Command.mName : Command.mName + " ");
            } else if (Prefix.starts_with(Name + " ")) {
                const std::string_view ArgumentPrefix{std::string_view{Prefix}.substr(Name.size() + 1)};

                for (const Core::FString& Argument : Command.mArguments) {
                    if (NormalizeConsolePrefix(Argument).starts_with(ArgumentPrefix)) {
                        Suggestions.push_back(Command.mName + " " + Argument);
                    }
                }
            }
        });

        return Suggestions;
    }

    /**
     * @brief 고정 명령과 동적 공급자가 반환한 명령을 순회합니다.
     */
    void Editor::FConsoleCommandRegistry::VisitCommands(const std::function<void(const Editor::FConsoleCommand&)>& Visit) const {
        for (const Editor::FConsoleCommand& Command : mCommands) {
            Visit(Command);
        }

        for (const Editor::FConsoleCommandRegistry::FCommandSource& Source : mSources) {
            for (const Editor::FConsoleCommand& Command : Source()) {
                Visit(Command);
            }
        }
    }

}
