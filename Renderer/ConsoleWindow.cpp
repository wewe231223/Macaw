#include "pch.h"
#include "ConsoleWindow.h"
#include "ImGui/imgui.h"

namespace
{
    const char* LogCategoryToString(ELogCategory Category)
    {
        switch (Category)
        {
        case ELogCategory::Core:    return "[Core]";
        case ELogCategory::Render:  return "[Render]";
        case ELogCategory::Physics: return "[Physics]";
        case ELogCategory::Etc: return "[Etc]";
        default:                    return "[Unknown]";
        }
    }
}

void DrawConsole(FConsoleOutputHandle Handle)
{
    ImGui::Begin("Console");
    const size_t Count = Console::GetMessageCount(Handle);

    for (size_t Index = 0; Index < Count; ++Index)
    {
        const FConsoleMessage& Message = Console::GetMessageAt(Handle, Index);

        ImGui::TextUnformatted(Message.Time.c_str());
        ImGui::SameLine();

        const char* LevelText = "[Log]";
        ImVec4 LevelColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        switch (Message.Level)
        {
        case ELogLevel::Warning:
            LevelText = "[Warning]";
            LevelColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
            break;

        case ELogLevel::Error:
            LevelText = "[Error]";
            LevelColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
            break;

        case ELogLevel::Fatal:
            LevelText = "[Fatal]";
            LevelColor = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
            break;

        default:
            break;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, LevelColor);
        ImGui::TextUnformatted(LevelText);
        ImGui::PopStyleColor();
        ImGui::SameLine();

        ImGui::TextUnformatted(LogCategoryToString(Message.Category));
        ImGui::SameLine();

        ImGui::TextUnformatted(Message.Text.c_str());
    }

    ImGui::End();
}

