/**
 * @file EEditorIcon.h
 * @brief 명령 정의와 에디터 UI가 공유하는 벡터 아이콘 식별자를 선언합니다.
 * @ingroup Editor
 */

#pragma once

namespace Editor {

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
        Grid,
        /**
         * @brief PIE 실행 시작 아이콘입니다.
         */
        Play,
        /**
         * @brief PIE 일시정지 아이콘입니다.
         */
        Pause,
        /**
         * @brief PIE 한 프레임 실행 아이콘입니다.
         */
        Step,
        /**
         * @brief PIE 실행 종료 아이콘입니다.
         */
        Stop,
        /**
         * @brief 4분할 뷰포트로 전환하는 아이콘입니다.
         */
        QuadView,
        /**
         * @brief 활성 뷰포트를 최대화하는 아이콘입니다.
         */
        Maximize,
        /**
         * @brief 재생 구간의 처음으로 이동하는 아이콘입니다.
         */
        JumpStart,
        /**
         * @brief 재생 구간의 끝으로 이동하는 아이콘입니다.
         */
        JumpEnd,
        /**
         * @brief 라이트를 나타내는 전구 아이콘입니다.
         */
        Light,
        /**
         * @brief 카메라를 나타내는 아이콘입니다.
         */
        Camera,
        /**
         * @brief 스켈레탈 메시를 나타내는 뼈 아이콘입니다.
         */
        Skeleton,
        /**
         * @brief 충돌 형상을 나타내는 캡슐 아이콘입니다.
         */
        Collision,
        /**
         * @brief 스프라이트와 빌보드를 나타내는 그림 아이콘입니다.
         */
        Sprite,
        /**
         * @brief 텍스트를 나타내는 T자 아이콘입니다.
         */
        Text,
        /**
         * @brief 안개를 나타내는 가로줄 아이콘입니다.
         */
        Fog,
        /**
         * @brief 레벨 시퀀스를 나타내는 필름 아이콘입니다.
         */
        Sequence,
        /**
         * @brief 특정 종류로 분류되지 않는 객체를 나타내는 마름모 아이콘입니다.
         */
        Component,
        /**
         * @brief 표시 중인 객체를 나타내는 눈 아이콘입니다.
         */
        Visible,
        /**
         * @brief 숨긴 객체를 나타내는 빗금 친 눈 아이콘입니다.
         */
        Hidden
    };

}
