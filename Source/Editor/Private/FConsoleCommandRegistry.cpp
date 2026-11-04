#include "Editor/FConsoleCommandRegistry.h"

#include <cctype>
#include <optional>
#include <utility>

namespace Editor {

    namespace {
        bool IsConsoleSpace(unsigned char Character) {
            return std::isspace(Character) != 0;
        }

        void SkipConsoleSpaces(std::string_view& Text) {
            while (!Text.empty() && IsConsoleSpace(Text.front())) {
                Text.remove_prefix(1);
            }
        }

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

    void Editor::FConsoleCommandRegistry::Register(Editor::FConsoleCommand Command) {
        mCommands.push_back(std::move(Command));
    }

    void Editor::FConsoleCommandRegistry::RegisterSource(Editor::FConsoleCommandRegistry::FCommandSource Source) {
        mSources.push_back(std::move(Source));
    }

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

    Core::TArray<Core::FString> Editor::FConsoleCommandRegistry::GetHelp() const {
        Core::TArray<Core::FString> Lines{};

        VisitCommands([&Lines](const Editor::FConsoleCommand& Command) {
            Lines.push_back(Command.mUsage.empty() ? Command.mName : Command.mName + " " + Command.mUsage);
        });

        return Lines;
    }

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
