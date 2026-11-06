#pragma once

#include "Core/Math/FVector2.h"
#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <bitset>

namespace Core {

    enum class EKey : Core::Int32 {
        None = 0,
        MouseLeft = 0x01,
        MouseRight = 0x02,
        MouseMiddle = 0x04,
        MouseX1 = 0x05,
        MouseX2 = 0x06,
        Backspace = 0x08,
        Tab = 0x09,
        Enter = 0x0D,
        Shift = 0x10,
        Control = 0x11,
        Alt = 0x12,
        Escape = 0x1B,
        Space = 0x20,
        PageUp = 0x21,
        PageDown = 0x22,
        End = 0x23,
        Home = 0x24,
        Left = 0x25,
        Up = 0x26,
        Right = 0x27,
        Down = 0x28,
        Insert = 0x2D,
        Delete = 0x2E,
        A = 'A',
        B = 'B',
        C = 'C',
        D = 'D',
        E = 'E',
        F = 'F',
        G = 'G',
        H = 'H',
        I = 'I',
        J = 'J',
        K = 'K',
        L = 'L',
        M = 'M',
        N = 'N',
        O = 'O',
        P = 'P',
        Q = 'Q',
        R = 'R',
        S = 'S',
        T = 'T',
        U = 'U',
        V = 'V',
        W = 'W',
        X = 'X',
        Y = 'Y',
        Z = 'Z',
        F1 = 0x70,
        LeftShift = 0xA0,
        RightShift = 0xA1,
        LeftControl = 0xA2,
        RightControl = 0xA3,
        LeftAlt = 0xA4,
        RightAlt = 0xA5,
        Comma = 0xBC
    };
    ReflectScriptEnum(EKey);

    enum class EInputRoute {
        Application,
        Game
    };

    class FInput final {
    private:
        using FKeys = std::bitset<256>;

    public:
        void BeginFrame();
        void SetFocused(bool Focused);
        void SetKey(Core::EKey Key, bool Down);
        void SetMousePosition(const Core::FVector2& Position);
        void AddMouseWheel(float Delta);
        void ReleaseMouse();
        void SetGameInputEnabled(bool Enabled);

        bool IsFocused() const;
        bool LostFocusThisFrame() const;
        bool IsGameInputEnabled() const;
        bool IsDown(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;
        bool WasPressed(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;
        bool WasReleased(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;
        Core::FVector2 GetMousePosition(Core::EInputRoute Route = Core::EInputRoute::Application) const;
        Core::FVector2 GetMouseDelta(Core::EInputRoute Route = Core::EInputRoute::Application) const;
        float GetMouseWheel(Core::EInputRoute Route = Core::EInputRoute::Application) const;

    private:
        bool CanRead(Core::EInputRoute Route) const;
        bool ReadKey(const FKeys& Keys, Core::EKey Key, Core::EInputRoute Route) const;

    private:
        FKeys mDown{};
        FKeys mPressed{};
        FKeys mReleased{};
        FKeys mGameBlockedKeys{};
        FKeys mGamePressed{};
        FKeys mGameReleased{};
        Core::FVector2 mMousePosition{};
        Core::FVector2 mMouseDelta{};
        float mMouseWheel{};
        bool mMouseReady{};
        bool mFocused{};
        bool mFocusLost{};
        bool mGameEnabled{};
        bool mGameFrameBlocked{};
    };

}
