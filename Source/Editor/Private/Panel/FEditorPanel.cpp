/**
 * @file FEditorPanel.cpp
 * @brief ImGui 패널의 이름, 가시성과 내용 그리기 수명을 관리합니다.
 * @ingroup Editor
 */

#include "Editor/Panel/FEditorPanel.h"

#include <imgui.h>
#include <utility>

/**
 * @brief FEditorPanel의 초기 상태와 전달받은 의존성을 구성합니다.
 */
FEditorPanel::FEditorPanel(UEditorEngine* Engine, FString Name, bool Closable)
	: mEngine{Engine},
	  mName{std::move(Name)},
	  mClosable{Closable} {
}

/**
 * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
 */
FEditorPanel::~FEditorPanel() = default;

/**
 * @brief 뷰포트 또는 패널의 현재 내용을 그립니다.
 */
void FEditorPanel::Draw(float DeltaTime) {
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
const FString& FEditorPanel::GetName() const {
    return mName;
}

/**
 * @brief 경계 상자가 뷰 프러스텀과 겹치는지 검사합니다.
 */
bool FEditorPanel::IsVisible() const {
    return mVisible;
}

/**
 * @brief 패널 창의 표시 상태를 설정합니다.
 */
void FEditorPanel::SetVisible(bool Visible) {
    mVisible = Visible;
}

/**
 * @brief 연결된 에디터 엔진을 반환합니다.
 */
UEditorEngine* FEditorPanel::GetEditorEngine() const {
    return mEngine;
}
