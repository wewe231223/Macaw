/**
 * @file FViewportInput.h
 * @brief 한 프레임의 뷰포트 마우스, 키보드 및 포커스 입력 스냅샷입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"

namespace Engine {

    /**
     * @brief 한 프레임의 뷰포트 마우스, 키보드 및 포커스 입력 스냅샷입니다.
     * @ingroup Engine
     */
    struct FViewportInput {
    public:
        /**
         * @brief 뷰포트 기준 마우스 픽셀 좌표입니다.
         */
        Core::FVector2 mMouse{};

        /**
         * @brief 이전 프레임 이후 마우스 이동량입니다.
         */
        Core::FVector2 mDelta{};

        /**
         * @brief 마우스 휠 이동량입니다.
         */
        float mWheel{};

        /**
         * @brief 마우스가 뷰포트 위에 있는지 여부입니다.
         */
        bool mHovered{};

        /**
         * @brief 뷰포트가 키보드 포커스를 갖는지 여부입니다.
         */
        bool mFocused{};

        /**
         * @brief 이번 프레임에 왼쪽 버튼을 눌렀는지 여부입니다.
         */
        bool mLeftPressed{};

        /**
         * @brief 왼쪽 버튼을 누르고 있는지 여부입니다.
         */
        bool mLeftDown{};

        /**
         * @brief 이번 프레임에 왼쪽 버튼을 놓았는지 여부입니다.
         */
        bool mLeftReleased{};

        /**
         * @brief 오른쪽 버튼을 누르고 있는지 여부입니다.
         */
        bool mRightDown{};

        /**
         * @brief 가운데 버튼을 누르고 있는지 여부입니다.
         */
        bool mMiddleDown{};

        /**
         * @brief Alt 키 상태입니다.
         */
        bool mAlt{};

        /**
         * @brief 이번 프레임에 Space 키를 눌러 변환 기즈모 모드를 순환하는 요청입니다.
         */
        bool mCycleGizmo{};

        /**
         * @brief 선택 대상에 카메라를 맞추는 요청입니다.
         */
        bool mFocusSelection{};

        /**
         * @brief 취소 키 입력입니다.
         */
        bool mEscape{};

        /**
         * @brief 삭제 키 입력입니다.
         */
        bool mDelete{};

        /**
         * @brief W 키를 누르는 동안 유지되는 카메라 전진 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mForward{};

        /**
         * @brief S 키를 누르는 동안 유지되는 카메라 후진 입력입니다.
         */
        bool mBackward{};

        /**
         * @brief A 키를 누르는 동안 유지되는 카메라 좌측 이동 입력입니다.
         */
        bool mLeft{};

        /**
         * @brief D 키를 누르는 동안 유지되는 카메라 우측 이동 입력입니다.
         */
        bool mRight{};

        /**
         * @brief E 키를 누르는 동안 유지되는 카메라 위 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mUp{};

        /**
         * @brief Q 키를 누르는 동안 유지되는 카메라 아래 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mDown{};

        /**
         * @brief 카메라 고속 이동 입력입니다.
         */
        bool mFast{};

        /**
         * @brief 현재 프레임에 가운데 마우스 버튼을 누른 상태입니다.
         */
        bool mMiddlePressed{};

        /**
         * @brief 현재 프레임에 가운데 마우스 버튼을 놓은 상태입니다.
         */
        bool mMiddleReleased{};

        /**
         * @brief Ctrl 키가 눌려 다중 선택을 요청하는 상태입니다.
         */
        bool mControl{};

        /**
         * @brief W 키로 이동 모드를 요청한 상태입니다.
         */
        bool mTranslateMode{};

        /**
         * @brief E 키로 회전 모드를 요청한 상태입니다.
         */
        bool mRotateMode{};

        /**
         * @brief R 키로 스케일 모드를 요청한 상태입니다.
         */
        bool mScaleMode{};

        /**
         * @brief Q 키로 선택 전용 모드를 요청한 상태입니다.
         */
        bool mSelectMode{};

        /**
         * @brief Tab 키로 월드와 로컬 좌표계 전환을 요청한 상태입니다.
         */
        bool mToggleSpace{};

        /**
         * @brief X 키로 현재 변환 모드의 스냅 전환을 요청한 상태입니다.
         */
        bool mToggleSnap{};
    };

}
