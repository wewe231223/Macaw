#pragma once

#include "Editor/Widget/FEditorWidget.h"
#include "Editor/FEditorContext.h"

namespace Editor {

    class FEditorContextWidget : public Editor::FEditorWidget {
    public:
        FEditorContextWidget(Editor::FEditorContext* Context, Core::FString Id);
        ~FEditorContextWidget() override;

    public:
        Editor::FEditorContext* GetEditorContext() const;
        void SetEditorContext(Editor::FEditorContext* Context);
        bool UsesContentRegion() const override;

    private:
        Editor::FEditorContext* mContext{};
    };
}
