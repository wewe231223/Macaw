/**
 * @file ViewportToolbar.h
 * @brief 뷰포트 클라이언트의 명령과 상태를 표시하는 툴바를 선언합니다.
 * @ingroup Editor
 */

#pragma once

class FEditorViewportClient;

/**
 * @brief 뷰포트의 변환 도구, 좌표계와 그리드 설정 UI를 구성합니다.
 * @ingroup Editor
 * @note 명령의 실행 가능 여부와 선택 상태는 클라이언트에 조회하며 편집 상태를 따로 저장하지 않습니다.
 */
namespace ViewportToolbar {
    /**
     * @brief 아이콘 버튼과 메뉴를 표시하고 사용자 요청을 클라이언트의 공통 명령 경로로 전달합니다.
     * @param[in,out] Client 명령 실행과 상태 조회에 사용할 뷰포트 클라이언트의 비소유 주소입니다.
     * @pre Client는 nullptr이 아니어야 하며 현재 ImGui 프레임과 창이 유효해야 합니다.
     * @note 그리드 설정 메뉴는 열 수 있지만 그리드 기능이 지원되기 전까지 항목은 비활성입니다.
     */
    void Draw(FEditorViewportClient* Client);
}
