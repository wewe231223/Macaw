/**
 * @file EViewportCommand.h
 * @brief 툴바, 메뉴와 단축키가 공유하는 뷰포트 명령을 정의합니다.
 * @ingroup Editor
 */

#pragma once

/**
 * @brief 뷰포트 클라이언트가 실행 여부와 선택 상태를 판단하는 동작 식별자입니다.
 * @ingroup Editor
 * @see FEditorViewportClient::ExecuteCommand
 * @see FEditorViewportClient::CanExecuteCommand
 * @see FEditorViewportClient::IsCommandChecked
 */
enum class EViewportCommand {
    /**
     * @brief 이동 기즈모 모드를 선택합니다.
     */
    Translate,
    /**
     * @brief 회전 기즈모 모드를 선택합니다.
     */
    Rotate,
    /**
     * @brief 스케일 기즈모 모드를 선택하고 좌표계를 로컬로 설정합니다.
     */
    Scale,
    /**
     * @brief 월드 좌표계를 선택하며 스케일 모드에서는 실행할 수 없습니다.
     */
    WorldSpace,
    /**
     * @brief 로컬 좌표계를 선택합니다.
     */
    LocalSpace,
    /**
     * @brief 월드와 로컬 좌표계를 전환하며 스케일 모드에서는 실행할 수 없습니다.
     */
    ToggleCoordinateSystem,
    /**
     * @brief 이동, 회전, 스케일 순서로 모드를 순환하며 스케일 진입 시 로컬 좌표계를 사용합니다.
     */
    CycleTransformMode,
    /**
     * @brief 향후 그리드 표시 전환에 사용할 명령입니다.
     * @note 현재 미구현이므로 실행 가능 여부와 선택 상태 조회는 false를 반환합니다.
     */
    ToggleGrid,
    Select
};
