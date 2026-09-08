#pragma once

class FConsolePanel : public IEditorPanel
{
public:
    explicit FConsolePanel(/* Console에 필요한 정보 */)
    {
    }

    void DrawPanel() override
    {
        DrawConsole(Console::STDOutHandle);
    }
};