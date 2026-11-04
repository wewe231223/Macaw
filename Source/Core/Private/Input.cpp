#include "Core/Input.h"

namespace Core {

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

    void FInput::SetMousePosition(const FVector2& Position) {
        if (mMouseReady) {
            mMouseDelta += Position - mMousePosition;
        }

        mMousePosition = Position;
        mMouseReady = true;
    }

    void FInput::AddMouseWheel(float Delta) {
        mMouseWheel += Delta;
    }

    void FInput::ReleaseMouse() {
        SetKey(EKey::MouseLeft, false);
        SetKey(EKey::MouseRight, false);
        SetKey(EKey::MouseMiddle, false);
        SetKey(EKey::MouseX1, false);
        SetKey(EKey::MouseX2, false);
        mMouseReady = false;
        mMouseDelta = {};
    }

    void FInput::SetGameInputEnabled(bool Enabled) {
        if (mGameEnabled == Enabled) {
            return;
        }

        mGameEnabled = Enabled;
        mGameBlockedKeys = Enabled ? mDown : FKeys{};
        mGameFrameBlocked = true;
    }

    bool FInput::IsFocused() const {
        return mFocused;
    }

    bool FInput::LostFocusThisFrame() const {
        return mFocusLost;
    }

    bool FInput::IsGameInputEnabled() const {
        return mGameEnabled && mFocused;
    }

    bool FInput::IsDown(EKey Key, EInputRoute Route) const {
        return ReadKey(mDown, Key, Route) && (Route == EInputRoute::Application || !mGameBlockedKeys[static_cast<std::size_t>(Key)]);
    }

    bool FInput::WasPressed(EKey Key, EInputRoute Route) const {
        return ReadKey(Route == EInputRoute::Game ? mGamePressed : mPressed, Key, Route);
    }

    bool FInput::WasReleased(EKey Key, EInputRoute Route) const {
        return ReadKey(Route == EInputRoute::Game ? mGameReleased : mReleased, Key, Route);
    }

    FVector2 FInput::GetMousePosition(EInputRoute Route) const {
        return CanRead(Route) ? mMousePosition : FVector2{};
    }

    FVector2 FInput::GetMouseDelta(EInputRoute Route) const {
        return CanRead(Route) ? mMouseDelta : FVector2{};
    }

    float FInput::GetMouseWheel(EInputRoute Route) const {
        return CanRead(Route) ? mMouseWheel : 0.0f;
    }

    bool FInput::CanRead(EInputRoute Route) const {
        return Route == EInputRoute::Application || (IsGameInputEnabled() && !mGameFrameBlocked);
    }

    bool FInput::ReadKey(const FKeys& Keys, EKey Key, EInputRoute Route) const {
        const auto Index{static_cast<std::size_t>(Key)};

        return Index < Keys.size() && CanRead(Route) && Keys[Index];
    }

}
