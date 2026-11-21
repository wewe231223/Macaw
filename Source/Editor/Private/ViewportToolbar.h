/**
 * @file ViewportToolbar.h
 * @brief 뷰포트 클라이언트의 명령과 상태를 표시하는 툴바를 선언합니다.
 * @ingroup Editor
 */

#pragma once

#include "Editor/EEditorViewportType.h"
#include "Editor/FEditorViewportClient.h"

#include <functional>

namespace Editor {

    /**
     * @brief 뷰포트의 변환 도구, 좌표계와 그리드 설정 UI를 구성합니다.
     * @ingroup Editor
     * @note 명령의 실행 가능 여부와 선택 상태는 클라이언트에 조회하며 편집 상태를 따로 저장하지 않습니다.
     */
    namespace ViewportToolbar {
        /**
         * @brief 아이콘 버튼과 메뉴를 표시하고 사용자 요청을 클라이언트의 공통 명령 경로로 전달합니다.
         * @param[in,out] Client 명령 실행과 상태 조회에 사용할 뷰포트 클라이언트의 비소유 주소입니다.
         * @param[in] Width 툴바 띠가 차지할 픽셀 너비이며 좁으면 일부 묶음을 압축 메뉴로 접습니다.
         * @param[in] AllowInput false이면 툴바 컨트롤을 비활성으로 표시합니다.
         * @param[in] PlayStatus PIE 중 컨트롤 대신 표시할 상태 문자열이며 nullptr이면 컨트롤을 그립니다.
         * @param[in] SetViewType 뷰 타입 메뉴에서 선택한 값을 적용할 콜백이며 비어 있으면 Client에 직접 설정합니다.
         * @param[in] DrawLayout 오른쪽 끝에 뷰포트 배치 컨트롤을 그릴 콜백이며 비어 있으면 배치 컨트롤을 생략합니다.
         * @pre Client는 nullptr이 아니어야 하며 현재 ImGui 프레임과 창이 유효해야 합니다.
         * @post 커서는 툴바 띠 바로 아래 왼쪽 끝으로 이동합니다.
         */
        void Draw(Editor::FEditorViewportClient* Client, float Width, bool AllowInput, const char* PlayStatus, const std::function<void(Editor::EEditorViewportType)>& SetViewType, const std::function<void()>& DrawLayout);
    }

}
