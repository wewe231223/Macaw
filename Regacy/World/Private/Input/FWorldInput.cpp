#include "pch.h"
#include "World/Input/FWorldInput.h"

EInputState FWorldInput::UpdateState(EInputState State, bool Down) {
    const bool WasDown{State == EInputState::Pressed || State == EInputState::Held};

    return Down ? (WasDown ? EInputState::Held : EInputState::Pressed) : (WasDown ? EInputState::Released : EInputState::None);
}

void FWorldInput::UpdateKey(Uint8 Key, bool Down) {
    mKeys[Key] = UpdateState(mKeys[Key], Down);
}

void FWorldInput::UpdateMouseButton(Uint8 Button, bool Down) {
    if (Button < mMouseButtons.size()) {
        mMouseButtons[Button] = UpdateState(mMouseButtons[Button], Down);
    }
}

void FWorldInput::SetMouseMotion(const FVector2& Position, const FVector2& Delta, float Wheel) {
    mMousePosition = Position;
    mMouseDelta = Delta;
    mMouseWheel = Wheel;
}

EInputState FWorldInput::GetKeyState(Uint8 Key) const {
    return mKeys[Key];
}

EInputState FWorldInput::GetMouseButtonState(Uint8 Button) const {
    return Button < mMouseButtons.size() ? mMouseButtons[Button] : EInputState::None;
}

bool FWorldInput::IsKeyDown(Uint8 Key) const {
    const EInputState State{GetKeyState(Key)};

    return State == EInputState::Pressed || State == EInputState::Held;
}

bool FWorldInput::IsMouseButtonDown(Uint8 Button) const {
    const EInputState State{GetMouseButtonState(Button)};

    return State == EInputState::Pressed || State == EInputState::Held;
}

const FVector2& FWorldInput::GetMousePosition() const {
    return mMousePosition;
}

const FVector2& FWorldInput::GetMouseDelta() const {
    return mMouseDelta;
}

float FWorldInput::GetMouseWheel() const {
    return mMouseWheel;
}

void FWorldInput::Reset() {
    mKeys.fill(EInputState::None);
    mMouseButtons.fill(EInputState::None);
    mMousePosition = {};
    mMouseDelta = {};
    mMouseWheel = 0.0f;
}
