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
    /**
     * @brief 선택 전용 모드로 전환하고 기즈모를 숨깁니다.
     */
    Select,
    /**
     * @brief 이동량을 지정한 거리 간격에 맞추는 스냅의 사용 여부를 전환합니다.
     */
    ToggleTranslationSnap,
    /**
     * @brief 회전량을 지정한 각도 간격에 맞추는 스냅의 사용 여부를 전환합니다.
     */
    ToggleRotationSnap,
    /**
     * @brief 스케일 변화량을 지정한 간격에 맞추는 스냅의 사용 여부를 전환합니다.
     */
    ToggleScaleSnap,
    /**
     * @brief 현재 이동·회전·스케일 모드에 대응하는 스냅의 사용 여부를 전환합니다.
     * @note 선택 전용 모드에서는 실행할 수 없습니다.
     */
    ToggleTransformSnap
};
