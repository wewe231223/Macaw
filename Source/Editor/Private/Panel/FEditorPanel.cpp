/**
 * @file FEditorPanel.cpp
 * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FEditorPanel.h"

#include <imgui.h>
#include <utility>

namespace Editor {

    /**
     * @brief FEditorPanel의 초기 상태와 전달받은 의존성을 구성합니다.
     */
    Editor::FEditorPanel::FEditorPanel(Editor::FEditorContext* Context, Core::FString Name, bool Closable)
		: mContext{Context},
		  mName{std::move(Name)},
		  mClosable{Closable} {
    }

    /**
     * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
     */
    Editor::FEditorPanel::~FEditorPanel() = default;

    /**
     * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
     */
    void Editor::FEditorPanel::Draw(float DeltaTime) {
        if (!mVisible) {
            return;
        }

        if (ImGui::Begin(mName.c_str(), mClosable ? &mVisible : nullptr)) {
            DrawContent(DeltaTime);
        }

        ImGui::End();
    }

    /**
     * @brief 이름 또는 표시 문자열을 반환합니다.
     */
    const Core::FString& Editor::FEditorPanel::GetName() const {
        return mName;
    }

    /**
     * @brief 패널 창의 표시 여부를 반환합니다.
     */
    bool Editor::FEditorPanel::IsVisible() const {
        return mVisible;
    }

    /**
     * @brief 패널 창의 표시 상태를 설정합니다.
     */
    void Editor::FEditorPanel::SetVisible(bool Visible) {
        mVisible = Visible;
    }

    /**
     * @brief 연결된 공통 편집 컨텍스트를 반환합니다.
     */
    Editor::FEditorContext* Editor::FEditorPanel::GetEditorContext() const {
        return mContext;
    }

    void Editor::FEditorPanel::SetEditorContext(Editor::FEditorContext* Context) {
        if (Context != nullptr) {
            mContext = Context;
        }
    }

}
