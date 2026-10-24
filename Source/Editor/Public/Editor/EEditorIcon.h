/**
 * @file EEditorIcon.h
 * @brief 명령 정의와 에디터 UI가 공유하는 벡터 아이콘 식별자를 선언합니다.
 * @ingroup Editor
 */

#pragma once

/**
 * @brief 공통 벡터 아이콘의 종류입니다.
 * @ingroup Editor
 */
enum class EEditorIcon {
    /**
     * @brief 변환 도구 메뉴를 나타내는 아이콘입니다.
     */
    Tools,
    /**
     * @brief 이동을 나타내는 축 화살표 아이콘입니다.
     */
    Translate,
    /**
     * @brief 회전을 나타내는 원형 화살표 아이콘입니다.
     */
    Rotate,
    /**
     * @brief 스케일을 나타내는 사각 끝점 아이콘입니다.
     */
    Scale,
    /**
     * @brief 월드 좌표계를 나타내는 지구본 아이콘입니다.
     */
    World,
    /**
     * @brief 로컬 좌표계를 나타내는 입방체 아이콘입니다.
     */
    Local,
    /**
     * @brief 그리드 설정을 나타내는 격자 아이콘입니다.
     */
    Grid
};
