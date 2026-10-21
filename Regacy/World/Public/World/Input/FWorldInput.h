#pragma once
#include <array>
#include "Core/Common.h"
#include "Math/FMath.h"

enum class EInputState : Uint8 {
    None,
    Pressed,
    Held,
    Released
};

class FWorldInput final {
public:
    FWorldInput() = default;

public:
    void UpdateKey(Uint8 Key, bool Down);
    void UpdateMouseButton(Uint8 Button, bool Down);
    void SetMouseMotion(const FVector2& Position, const FVector2& Delta, float Wheel);
    EInputState GetKeyState(Uint8 Key) const;
    EInputState GetMouseButtonState(Uint8 Button) const;
    bool IsKeyDown(Uint8 Key) const;
    bool IsMouseButtonDown(Uint8 Button) const;
    const FVector2& GetMousePosition() const;
    const FVector2& GetMouseDelta() const;
    float GetMouseWheel() const;
    void Reset();

private:
    static EInputState UpdateState(EInputState State, bool Down);

private:
    std::array<EInputState, 256> mKeys{};
    std::array<EInputState, 5> mMouseButtons{};
    FVector2 mMousePosition{};
    FVector2 mMouseDelta{};
    float mMouseWheel{};
};
