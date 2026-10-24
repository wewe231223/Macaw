/**
 * @file FViewportInput.h
 * @brief 한 프레임의 뷰포트 마우스, 키보드 및 포커스 입력 스냅샷입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"

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
    FVector2 mMouse{};
    ReflectProperty(mMouse, "Mouse", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이전 프레임 이후 마우스 이동량입니다.
     */
    FVector2 mDelta{};
    ReflectProperty(mDelta, "Delta", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 마우스 휠 이동량입니다.
     */
    float mWheel{};
    ReflectProperty(mWheel, "Wheel", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 마우스가 뷰포트 위에 있는지 여부입니다.
     */
    bool mHovered{};
    ReflectProperty(mHovered, "Hovered", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 뷰포트가 키보드 포커스를 갖는지 여부입니다.
     */
    bool mFocused{};
    ReflectProperty(mFocused, "Focused", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 이번 프레임에 왼쪽 버튼을 눌렀는지 여부입니다.
     */
    bool mLeftPressed{};
    ReflectProperty(mLeftPressed, "LeftPressed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 왼쪽 버튼을 누르고 있는지 여부입니다.
     */
    bool mLeftDown{};
    ReflectProperty(mLeftDown, "LeftDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 왼쪽 버튼을 놓았는지 여부입니다.
     */
    bool mLeftReleased{};
    ReflectProperty(mLeftReleased, "LeftReleased", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 오른쪽 버튼을 누르고 있는지 여부입니다.
     */
    bool mRightDown{};
    ReflectProperty(mRightDown, "RightDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 가운데 버튼을 누르고 있는지 여부입니다.
     */
    bool mMiddleDown{};
    ReflectProperty(mMiddleDown, "MiddleDown", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief Alt 키 상태입니다.
     */
    bool mAlt{};
    ReflectProperty(mAlt, "Alt", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 이번 프레임에 Space 키를 눌러 변환 기즈모 모드를 순환하는 요청입니다.
     */
    bool mCycleGizmo{};
    ReflectProperty(mCycleGizmo, "CycleGizmo", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 선택 대상에 카메라를 맞추는 요청입니다.
     */
    bool mFocusSelection{};
    ReflectProperty(mFocusSelection, "FocusSelection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 취소 키 입력입니다.
     */
    bool mEscape{};
    ReflectProperty(mEscape, "Escape", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 삭제 키 입력입니다.
     */
    bool mDelete{};
    ReflectProperty(mDelete, "Delete", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief W 키를 누르는 동안 유지되는 카메라 전진 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
     */
    bool mForward{};
    ReflectProperty(mForward, "Forward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief S 키를 누르는 동안 유지되는 카메라 후진 입력입니다.
     */
    bool mBackward{};
    ReflectProperty(mBackward, "Backward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief A 키를 누르는 동안 유지되는 카메라 좌측 이동 입력입니다.
     */
    bool mLeft{};
    ReflectProperty(mLeft, "Left", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief D 키를 누르는 동안 유지되는 카메라 우측 이동 입력입니다.
     */
    bool mRight{};
    ReflectProperty(mRight, "Right", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief E 키를 누르는 동안 유지되는 카메라 위 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
     */
    bool mUp{};
    ReflectProperty(mUp, "Up", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief Q 키를 누르는 동안 유지되는 카메라 아래 이동 입력이며 오른쪽 마우스 버튼을 누른 비행 조작에서 사용합니다.
     */
    bool mDown{};
    ReflectProperty(mDown, "Down", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 고속 이동 입력입니다.
     */
    bool mFast{};
    ReflectProperty(mFast, "Fast", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 가운데 마우스 버튼을 눌렀는지 여부이며 이동 기즈모의 피벗 드래그를 시작할 때 사용합니다.
     */
    bool mMiddlePressed{};
    ReflectProperty(mMiddlePressed, "MiddlePressed", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 가운데 마우스 버튼을 놓았는지 여부이며 피벗 드래그의 마지막 위치를 적용하고 종료할 때 사용합니다.
     */
    bool mMiddleReleased{};
    ReflectProperty(mMiddleReleased, "MiddleReleased", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief Ctrl 키가 눌려 있는 상태이며 물체 클릭 시 다중 선택을 전환할 때 사용합니다.
     */
    bool mControl{};
    ReflectProperty(mControl, "Control", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 W 키를 눌러 이동 기즈모 모드로 전환하는 요청이며 카메라 비행 중에는 처리하지 않습니다.
     */
    bool mTranslateMode{};
    ReflectProperty(mTranslateMode, "TranslateMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 E 키를 눌러 회전 기즈모 모드로 전환하는 요청이며 카메라 비행 중에는 처리하지 않습니다.
     */
    bool mRotateMode{};
    ReflectProperty(mRotateMode, "RotateMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 R 키를 눌러 스케일 기즈모 모드로 전환하는 요청입니다.
     */
    bool mScaleMode{};
    ReflectProperty(mScaleMode, "ScaleMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 Q 키를 눌러 기즈모를 숨기는 선택 전용 모드로 전환하는 요청이며 카메라 비행 중에는 처리하지 않습니다.
     */
    bool mSelectMode{};
    ReflectProperty(mSelectMode, "SelectMode", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 Tab 키를 눌러 기즈모의 월드·로컬 좌표계 전환을 요청한 상태입니다.
     */
    bool mToggleSpace{};
    ReflectProperty(mToggleSpace, "ToggleSpace", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 이번 프레임에 X 키를 눌러 현재 변환 모드의 스냅 사용 여부 전환을 요청한 상태입니다.
     */
    bool mToggleSnap{};
    ReflectProperty(mToggleSnap, "ToggleSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

};
