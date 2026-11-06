/**
 * @file Input.cpp
 * @brief 플랫폼 입력을 프레임별 키 전이·마우스 상태로 저장하고 애플리케이션·게임 경로에 제공합니다.
 * @ingroup Core
 */

#include "Core/Input.h"

namespace Core {

    /**
     * @brief 새 프레임의 키 전이·마우스 이동·휠과 포커스 상실 기록을 초기화합니다.
     */
    void FInput::BeginFrame() {
        mPressed.reset();
        mReleased.reset();
        mGamePressed.reset();
        mGameReleased.reset();
        mMouseDelta = {};
        mMouseWheel = {};
        mFocusLost = false;
        mGameFrameBlocked = false;
    }

    /**
     * @brief 창의 포커스를 갱신하고 포커스를 잃으면 눌린 키와 게임 입력을 해제합니다.
     */
    void FInput::SetFocused(bool Focused) {
        if (mFocused == Focused) {
            return;
        }

        mFocused = Focused;
        mMouseReady = false;
        mMouseDelta = {};
        mMouseWheel = {};

        if (!Focused) {
            mFocusLost = true;
            mReleased |= mDown;

            mDown.reset();
            mPressed.reset();
            mGameBlockedKeys.reset();
            mGameFrameBlocked = true;
        }
    }

    /**
     * @brief 플랫폼에서 받은 키 상태를 반영하고 누름·해제 전이를 기록합니다.
     */
    void FInput::SetKey(EKey Key, bool Down) {
        const auto Index{static_cast<std::size_t>(Key)};

        if (Index == 0 || Index >= mDown.size() || mDown[Index] == Down || (Down && !mFocused)) {
            return;
        }

        mDown[Index] = Down;

        if (Down) {
            mPressed.set(Index);

            if (CanRead(EInputRoute::Game)) {
                mGamePressed.set(Index);
            }
        } else {
            mReleased.set(Index);

            if (CanRead(EInputRoute::Game) && !mGameBlockedKeys[Index]) {
                mGameReleased.set(Index);
            }

            mGameBlockedKeys.reset(Index);
        }
    }

    /**
     * @brief 새 마우스 좌표를 저장하고 이전 좌표와의 차이를 프레임 이동량에 누적합니다.
     */
    void FInput::SetMousePosition(const FVector2& Position) {
        if (mMouseReady) {
            mMouseDelta += Position - mMousePosition;
        }

        mMousePosition = Position;
        mMouseReady = true;
    }

    /**
     * @brief 현재 프레임의 마우스 휠 이동량을 누적합니다.
     */
    void FInput::AddMouseWheel(float Delta) {
        mMouseWheel += Delta;
    }

    /**
     * @brief 모든 마우스 버튼을 해제하고 위치 차이 누적을 초기화합니다.
     */
    void FInput::ReleaseMouse() {
        SetKey(EKey::MouseLeft, false);
        SetKey(EKey::MouseRight, false);
        SetKey(EKey::MouseMiddle, false);
        SetKey(EKey::MouseX1, false);
        SetKey(EKey::MouseX2, false);

        mMouseReady = false;
        mMouseDelta = {};
    }

    /**
     * @brief 게임 입력을 전환하고 전환 당시 눌린 키가 해제될 때까지 게임 전달을 막습니다.
     */
    void FInput::SetGameInputEnabled(bool Enabled) {
        if (mGameEnabled == Enabled) {
            return;
        }

        mGameEnabled = Enabled;
        mGameBlockedKeys = Enabled ? mDown : FKeys{};
        mGameFrameBlocked = true;
    }

    /**
     * @brief 애플리케이션 창의 입력 포커스 상태를 반환합니다.
     */
    bool FInput::IsFocused() const {
        return mFocused;
    }

    /**
     * @brief 현재 프레임에 창이 입력 포커스를 잃었는지 검사합니다.
     */
    bool FInput::LostFocusThisFrame() const {
        return mFocusLost;
    }

    /**
     * @brief 창에 포커스가 있고 게임 입력 경로가 활성화되었는지 검사합니다.
     */
    bool FInput::IsGameInputEnabled() const {
        return mGameEnabled && mFocused;
    }

    /**
     * @brief 입력 경로에서 해당 키를 누르고 있는지 검사합니다.
     */
    bool FInput::IsDown(EKey Key, EInputRoute Route) const {
        return ReadKey(mDown, Key, Route) && (Route == EInputRoute::Application || !mGameBlockedKeys[static_cast<std::size_t>(Key)]);
    }

    /**
     * @brief 현재 프레임에 해당 키의 누름 전이가 입력 경로에 기록되었는지 검사합니다.
     */
    bool FInput::WasPressed(EKey Key, EInputRoute Route) const {
        return ReadKey(Route == EInputRoute::Game ? mGamePressed : mPressed, Key, Route);
    }

    /**
     * @brief 현재 프레임에 해당 키의 해제 전이가 입력 경로에 기록되었는지 검사합니다.
     */
    bool FInput::WasReleased(EKey Key, EInputRoute Route) const {
        return ReadKey(Route == EInputRoute::Game ? mGameReleased : mReleased, Key, Route);
    }

    /**
     * @brief 요청한 입력 경로의 마지막 마우스 좌표를 반환합니다.
     */
    FVector2 FInput::GetMousePosition(EInputRoute Route) const {
        return CanRead(Route) ? mMousePosition : FVector2{};
    }

    /**
     * @brief 요청한 입력 경로의 프레임 누적 마우스 이동량을 반환합니다.
     */
    FVector2 FInput::GetMouseDelta(EInputRoute Route) const {
        return CanRead(Route) ? mMouseDelta : FVector2{};
    }

    /**
     * @brief 요청한 입력 경로의 프레임 누적 휠 이동량을 반환합니다.
     */
    float FInput::GetMouseWheel(EInputRoute Route) const {
        return CanRead(Route) ? mMouseWheel : 0.0f;
    }

    /**
     * @brief 입력 경로의 활성 상태와 전환 프레임 차단 여부를 검사합니다.
     */
    bool FInput::CanRead(EInputRoute Route) const {
        return Route == EInputRoute::Application || (IsGameInputEnabled() && !mGameFrameBlocked);
    }

    /**
     * @brief 키 번호와 입력 경로를 검증한 뒤 지정한 키 비트 집합을 조회합니다.
     */
    bool FInput::ReadKey(const FKeys& Keys, EKey Key, EInputRoute Route) const {
        const auto Index{static_cast<std::size_t>(Key)};

        return Index < Keys.size() && CanRead(Route) && Keys[Index];
    }

}
