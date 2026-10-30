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
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FViewportInput);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FViewportInput);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 뷰포트 기준 마우스 픽셀 좌표입니다.
         */
        Core::FVector2 mMouse{};
        ReflectProperty(mMouse, "Mouse", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이전 프레임 이후 마우스 이동량입니다.
         */
        Core::FVector2 mDelta{};
        ReflectProperty(mDelta, "Delta", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 마우스 휠 이동량입니다.
         */
        float mWheel{};
        ReflectProperty(mWheel, "Wheel", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 마우스가 뷰포트 위에 있는지 여부입니다.
         */
        bool mHovered{};
        ReflectProperty(mHovered, "Hovered", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뷰포트가 키보드 포커스를 갖는지 여부입니다.
         */
        bool mFocused{};
        ReflectProperty(mFocused, "Focused", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 이번 프레임에 왼쪽 버튼을 눌렀는지 여부입니다.
         */
        bool mLeftPressed{};
        ReflectProperty(mLeftPressed, "LeftPressed", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 왼쪽 버튼을 누르고 있는지 여부입니다.
         */
        bool mLeftDown{};
        ReflectProperty(mLeftDown, "LeftDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이번 프레임에 왼쪽 버튼을 놓았는지 여부입니다.
         */
        bool mLeftReleased{};
        ReflectProperty(mLeftReleased, "LeftReleased", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 오른쪽 버튼을 누르고 있는지 여부입니다.
         */
        bool mRightDown{};
        ReflectProperty(mRightDown, "RightDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가운데 버튼을 누르고 있는지 여부입니다.
         */
        bool mMiddleDown{};
        ReflectProperty(mMiddleDown, "MiddleDown", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief Alt 키 상태입니다.
         */
        bool mAlt{};
        ReflectProperty(mAlt, "Alt", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 이번 프레임에 Space 키를 눌러 변환 기즈모 모드를 순환하는 요청입니다.
         */
        bool mCycleGizmo{};
        ReflectProperty(mCycleGizmo, "CycleGizmo", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 선택 대상에 카메라를 맞추는 요청입니다.
         */
        bool mFocusSelection{};
        ReflectProperty(mFocusSelection, "FocusSelection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 취소 키 입력입니다.
         */
        bool mEscape{};
        ReflectProperty(mEscape, "Escape", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 삭제 키 입력입니다.
         */
        bool mDelete{};
        ReflectProperty(mDelete, "Delete", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief W 키를 누르는 동안 유지되는 카메라 전진 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mForward{};
        ReflectProperty(mForward, "Forward", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief S 키를 누르는 동안 유지되는 카메라 후진 입력입니다.
         */
        bool mBackward{};
        ReflectProperty(mBackward, "Backward", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief A 키를 누르는 동안 유지되는 카메라 좌측 이동 입력입니다.
         */
        bool mLeft{};
        ReflectProperty(mLeft, "Left", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief D 키를 누르는 동안 유지되는 카메라 우측 이동 입력입니다.
         */
        bool mRight{};
        ReflectProperty(mRight, "Right", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief E 키를 누르는 동안 유지되는 카메라 위 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mUp{};
        ReflectProperty(mUp, "Up", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Q 키를 누르는 동안 유지되는 카메라 아래 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
         */
        bool mDown{};
        ReflectProperty(mDown, "Down", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라 고속 이동 입력입니다.
         */
        bool mFast{};
        ReflectProperty(mFast, "Fast", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프레임에 가운데 마우스 버튼을 누른 상태입니다.
         */
        bool mMiddlePressed{};
        ReflectProperty(mMiddlePressed, "MiddlePressed", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 프레임에 가운데 마우스 버튼을 놓은 상태입니다.
         */
        bool mMiddleReleased{};
        ReflectProperty(mMiddleReleased, "MiddleReleased", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Ctrl 키가 눌려 다중 선택을 요청하는 상태입니다.
         */
        bool mControl{};
        ReflectProperty(mControl, "Control", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief W 키로 이동 모드를 요청한 상태입니다.
         */
        bool mTranslateMode{};
        ReflectProperty(mTranslateMode, "TranslateMode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief E 키로 회전 모드를 요청한 상태입니다.
         */
        bool mRotateMode{};
        ReflectProperty(mRotateMode, "RotateMode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief R 키로 스케일 모드를 요청한 상태입니다.
         */
        bool mScaleMode{};
        ReflectProperty(mScaleMode, "ScaleMode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Q 키로 선택 전용 모드를 요청한 상태입니다.
         */
        bool mSelectMode{};
        ReflectProperty(mSelectMode, "SelectMode", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief Tab 키로 월드와 로컬 좌표계 전환을 요청한 상태입니다.
         */
        bool mToggleSpace{};
        ReflectProperty(mToggleSpace, "ToggleSpace", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief X 키로 현재 변환 모드의 스냅 전환을 요청한 상태입니다.
         */
        bool mToggleSnap{};
        ReflectProperty(mToggleSnap, "ToggleSnap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

    };

}
