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
     * @brief 기즈모 모드 전환 요청입니다.
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
     * @brief 카메라 전진 입력입니다.
     */
    bool mForward{};
    ReflectProperty(mForward, "Forward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 후진 입력입니다.
     */
    bool mBackward{};
    ReflectProperty(mBackward, "Backward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 좌측 이동 입력입니다.
     */
    bool mLeft{};
    ReflectProperty(mLeft, "Left", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 우측 이동 입력입니다.
     */
    bool mRight{};
    ReflectProperty(mRight, "Right", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 위 이동 입력입니다.
     */
    bool mUp{};
    ReflectProperty(mUp, "Up", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 아래 이동 입력입니다.
     */
    bool mDown{};
    ReflectProperty(mDown, "Down", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라 고속 이동 입력입니다.
     */
    bool mFast{};
    ReflectProperty(mFast, "Fast", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
