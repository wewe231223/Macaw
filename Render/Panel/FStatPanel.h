#pragme once

class FStatPanel : public IEditorPanel
{
public:
    explicit FStatPanel(UWorld& InWorld)
        : World(&InWorld)
    {
    }

    void DrawPanel() override
    {
        DrawStatWindow(*World);
    }

private:
    UWorld* World = nullptr;
};