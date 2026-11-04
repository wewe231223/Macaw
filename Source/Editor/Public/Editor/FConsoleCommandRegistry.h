#pragma once

#include "Core/Types.h"

#include <functional>
#include <string_view>

namespace Editor {

    struct FConsoleInvocation {
        std::string_view mName{};
        std::string_view mUsage{};
        std::string_view mInput{};
        std::string_view mArguments{};
    };

    struct FConsoleCommand {
        Core::FString mName{};
        Core::FString mUsage{};
        Core::TArray<Core::FString> mArguments{};
        std::function<void(const Editor::FConsoleInvocation&)> mExecute{};
    };

    class FConsoleCommandRegistry {
    public:
        using FCommandSource = std::function<Core::TArray<Editor::FConsoleCommand>()>;

    public:
        void Register(Editor::FConsoleCommand Command);
        void RegisterSource(Editor::FConsoleCommandRegistry::FCommandSource Source);

        bool Execute(std::string_view Input) const;
        Core::TArray<Core::FString> GetHelp() const;
        Core::TArray<Core::FString> GetSuggestions(std::string_view Input) const;

    private:
        void VisitCommands(const std::function<void(const Editor::FConsoleCommand&)>& Visit) const;

    private:
        Core::TArray<Editor::FConsoleCommand> mCommands{};
        Core::TArray<Editor::FConsoleCommandRegistry::FCommandSource> mSources{};
    };

}
