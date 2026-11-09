/**
 * @file Input.h
 * @brief 플랫폼 입력을 프레임별 키 전이·마우스 상태로 저장하고 애플리케이션·게임 경로에 제공합니다.
 * @ingroup Core
 */

#pragma once

#include "Core/Math/FVector2.h"
#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <bitset>

namespace Core {

    /**
     * @brief Win32 가상 키 값과 일치하는 키보드·마우스 버튼 식별자입니다.
     */
    UENUM(ScriptOnly)
    enum class EKey : Core::Int32 {
        /**
         * @brief 유효한 키가 없는 상태입니다.
         */
        None = 0,

        /**
         * @brief 마우스 왼쪽 버튼입니다.
         */
        MouseLeft = 0x01,

        /**
         * @brief 마우스 오른쪽 버튼입니다.
         */
        MouseRight = 0x02,

        /**
         * @brief 마우스 가운데 버튼입니다.
         */
        MouseMiddle = 0x04,

        /**
         * @brief 마우스 첫 번째 추가 버튼입니다.
         */
        MouseX1 = 0x05,

        /**
         * @brief 마우스 두 번째 추가 버튼입니다.
         */
        MouseX2 = 0x06,

        /**
         * @brief 백스페이스 키입니다.
         */
        Backspace = 0x08,

        /**
         * @brief Tab 키입니다.
         */
        Tab = 0x09,

        /**
         * @brief 엔터 키입니다.
         */
        Enter = 0x0D,

        /**
         * @brief 좌우를 구분하지 않는 Shift 키입니다.
         */
        Shift = 0x10,

        /**
         * @brief 좌우를 구분하지 않는 Control 키입니다.
         */
        Control = 0x11,

        /**
         * @brief 좌우를 구분하지 않는 Alt 키입니다.
         */
        Alt = 0x12,

        /**
         * @brief Escape 키입니다.
         */
        Escape = 0x1B,

        /**
         * @brief 스페이스 키입니다.
         */
        Space = 0x20,

        /**
         * @brief PageUp 키입니다.
         */
        PageUp = 0x21,

        /**
         * @brief PageDown 키입니다.
         */
        PageDown = 0x22,

        /**
         * @brief End 키입니다.
         */
        End = 0x23,

        /**
         * @brief Home 키입니다.
         */
        Home = 0x24,

        /**
         * @brief 왼쪽 방향키입니다.
         */
        Left = 0x25,

        /**
         * @brief 위 방향키입니다.
         */
        Up = 0x26,

        /**
         * @brief 오른쪽 방향키입니다.
         */
        Right = 0x27,

        /**
         * @brief 아래 방향키입니다.
         */
        Down = 0x28,

        /**
         * @brief Insert 키입니다.
         */
        Insert = 0x2D,

        /**
         * @brief Delete 키입니다.
         */
        Delete = 0x2E,

        /**
         * @brief 영문 A 키입니다.
         */
        A = 'A',

        /**
         * @brief 영문 B 키입니다.
         */
        B = 'B',

        /**
         * @brief 영문 C 키입니다.
         */
        C = 'C',

        /**
         * @brief 영문 D 키입니다.
         */
        D = 'D',

        /**
         * @brief 영문 E 키입니다.
         */
        E = 'E',

        /**
         * @brief 영문 F 키입니다.
         */
        F = 'F',

        /**
         * @brief 영문 G 키입니다.
         */
        G = 'G',

        /**
         * @brief 영문 H 키입니다.
         */
        H = 'H',

        /**
         * @brief 영문 I 키입니다.
         */
        I = 'I',

        /**
         * @brief 영문 J 키입니다.
         */
        J = 'J',

        /**
         * @brief 영문 K 키입니다.
         */
        K = 'K',

        /**
         * @brief 영문 L 키입니다.
         */
        L = 'L',

        /**
         * @brief 영문 M 키입니다.
         */
        M = 'M',

        /**
         * @brief 영문 N 키입니다.
         */
        N = 'N',

        /**
         * @brief 영문 O 키입니다.
         */
        O = 'O',

        /**
         * @brief 영문 P 키입니다.
         */
        P = 'P',

        /**
         * @brief 영문 Q 키입니다.
         */
        Q = 'Q',

        /**
         * @brief 영문 R 키입니다.
         */
        R = 'R',

        /**
         * @brief 영문 S 키입니다.
         */
        S = 'S',

        /**
         * @brief 영문 T 키입니다.
         */
        T = 'T',

        /**
         * @brief 영문 U 키입니다.
         */
        U = 'U',

        /**
         * @brief 영문 V 키입니다.
         */
        V = 'V',

        /**
         * @brief 영문 W 키입니다.
         */
        W = 'W',

        /**
         * @brief 영문 X 키입니다.
         */
        X = 'X',

        /**
         * @brief 영문 Y 키입니다.
         */
        Y = 'Y',

        /**
         * @brief 영문 Z 키입니다.
         */
        Z = 'Z',

        /**
         * @brief F1 키입니다.
         */
        F1 = 0x70,

        /**
         * @brief 왼쪽 Shift 키입니다.
         */
        LeftShift = 0xA0,

        /**
         * @brief 오른쪽 Shift 키입니다.
         */
        RightShift = 0xA1,

        /**
         * @brief 왼쪽 Control 키입니다.
         */
        LeftControl = 0xA2,

        /**
         * @brief 오른쪽 Control 키입니다.
         */
        RightControl = 0xA3,

        /**
         * @brief 왼쪽 Alt 키입니다.
         */
        LeftAlt = 0xA4,

        /**
         * @brief 오른쪽 Alt 키입니다.
         */
        RightAlt = 0xA5,

        /**
         * @brief 쉼표 키입니다.
         */
        Comma = 0xBC
    };

    /**
     * @brief 애플리케이션 원본 입력과 활성 게임에 전달할 입력을 구분합니다.
     */
    enum class EInputRoute {
        /**
         * @brief UI를 포함한 애플리케이션 원본 입력 경로입니다.
         */
        Application,

        /**
         * @brief 포커스·활성 상태를 검사하는 게임 입력 경로입니다.
         */
        Game
    };

    /**
     * @brief 플랫폼 입력을 프레임별 키 전이·마우스 상태로 저장하고 애플리케이션·게임 경로에 제공합니다.
     */
    class FInput final {
    private:
        /**
         * @brief Win32 가상 키 범위의 상태를 저장하는 256비트 집합입니다.
         */
        using FKeys = std::bitset<256>;

    public:
        /**
         * @brief 새 프레임의 키 전이·마우스 이동·휠과 포커스 상실 기록을 초기화합니다.
         */
        void BeginFrame();

        /**
         * @brief 창의 포커스를 갱신하고 포커스를 잃으면 눌린 키와 게임 입력을 해제합니다.
         * @param[in] Focused 창의 입력 포커스 여부입니다.
         */
        void SetFocused(bool Focused);

        /**
         * @brief 플랫폼에서 받은 키 상태를 반영하고 누름·해제 전이를 기록합니다.
         * @param[in] Key 조회하거나 갱신할 키입니다.
         * @param[in] Down 누름 상태이면 true, 해제이면 false입니다.
         */
        void SetKey(Core::EKey Key, bool Down);

        /**
         * @brief 새 마우스 좌표를 저장하고 이전 좌표와의 차이를 프레임 이동량에 누적합니다.
         * @param[in] Position 새 마우스 좌표입니다.
         */
        void SetMousePosition(const Core::FVector2& Position);

        /**
         * @brief 현재 프레임의 마우스 휠 이동량을 누적합니다.
         * @param[in] Delta 추가할 휠 이동량입니다.
         */
        void AddMouseWheel(float Delta);

        /**
         * @brief 모든 마우스 버튼을 해제하고 위치 차이 누적을 초기화합니다.
         */
        void ReleaseMouse();

        /**
         * @brief 게임 입력을 전환하고 전환 당시 눌린 키가 해제될 때까지 게임 전달을 막습니다.
         * @param[in] Enabled 해당 입력 경로 또는 스냅의 활성 여부입니다.
         */
        void SetGameInputEnabled(bool Enabled);

        /**
         * @brief 애플리케이션 창의 입력 포커스 상태를 반환합니다.
         * @return 창이 포커스를 가지고 있으면 true입니다.
         */
        bool IsFocused() const;

        /**
         * @brief 현재 프레임에 창이 입력 포커스를 잃었는지 검사합니다.
         * @return 이번 프레임에 포커스를 잃었으면 true입니다.
         */
        bool LostFocusThisFrame() const;

        /**
         * @brief 창에 포커스가 있고 게임 입력 경로가 활성화되었는지 검사합니다.
         * @return 게임 입력을 켰고 창에 포커스가 있으면 true입니다.
         */
        bool IsGameInputEnabled() const;

        /**
         * @brief 입력 경로에서 해당 키를 누르고 있는지 검사합니다.
         * @param[in] Key 조회하거나 갱신할 키입니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 입력 경로에서 키가 눌렸고 차단되지 않았으면 true입니다.
         */
        bool IsDown(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;

        /**
         * @brief 현재 프레임에 해당 키의 누름 전이가 입력 경로에 기록되었는지 검사합니다.
         * @param[in] Key 조회하거나 갱신할 키입니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 이번 프레임에 해당 경로에서 키를 새로 눌렀으면 true입니다.
         */
        bool WasPressed(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;

        /**
         * @brief 현재 프레임에 해당 키의 해제 전이가 입력 경로에 기록되었는지 검사합니다.
         * @param[in] Key 조회하거나 갱신할 키입니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 이번 프레임에 해당 경로에서 키를 해제했으면 true입니다.
         */
        bool WasReleased(Core::EKey Key, Core::EInputRoute Route = Core::EInputRoute::Application) const;

        /**
         * @brief 요청한 입력 경로의 마지막 마우스 좌표를 반환합니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 요청 경로의 좌표이며 차단되면 영벡터입니다.
         */
        Core::FVector2 GetMousePosition(Core::EInputRoute Route = Core::EInputRoute::Application) const;

        /**
         * @brief 요청한 입력 경로의 프레임 누적 마우스 이동량을 반환합니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 요청 경로의 이동량이며 차단되면 영벡터입니다.
         */
        Core::FVector2 GetMouseDelta(Core::EInputRoute Route = Core::EInputRoute::Application) const;

        /**
         * @brief 요청한 입력 경로의 프레임 누적 휠 이동량을 반환합니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 요청 경로의 휠 이동량이며 차단되면 0입니다.
         */
        float GetMouseWheel(Core::EInputRoute Route = Core::EInputRoute::Application) const;

    private:
        /**
         * @brief 입력 경로의 활성 상태와 전환 프레임 차단 여부를 검사합니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 현재 프레임에 요청 경로를 읽을 수 있으면 true입니다.
         */
        bool CanRead(Core::EInputRoute Route) const;

        /**
         * @brief 키 번호와 입력 경로를 검증한 뒤 지정한 키 비트 집합을 조회합니다.
         * @param[in] Keys 키 상태를 읽을 비트 집합입니다.
         * @param[in] Key 조회하거나 갱신할 키입니다.
         * @param[in] Route 조회할 애플리케이션·게임 입력 경로입니다.
         * @return 키 번호·경로가 유효하고 비트가 켜졌으면 true입니다.
         */
        bool ReadKey(const FKeys& Keys, Core::EKey Key, Core::EInputRoute Route) const;

    private:
        /**
         * @brief 현재 눌려 있는 키의 비트 집합입니다.
         */
        FKeys mDown{};

        /**
         * @brief 이번 프레임에 눌린 키입니다.
         */
        FKeys mPressed{};

        /**
         * @brief 이번 프레임에 해제된 키입니다.
         */
        FKeys mReleased{};

        /**
         * @brief 게임 입력을 켤 때 눌려 있어 해제 전까지 전달하지 않는 키입니다.
         */
        FKeys mGameBlockedKeys{};

        /**
         * @brief 게임 경로의 이번 프레임 키 누름 전이입니다.
         */
        FKeys mGamePressed{};

        /**
         * @brief 게임 경로의 이번 프레임 키 해제 전이입니다.
         */
        FKeys mGameReleased{};

        /**
         * @brief 마지막 수신 마우스 좌표입니다.
         */
        Core::FVector2 mMousePosition{};

        /**
         * @brief 이번 프레임의 누적 마우스 이동량입니다.
         */
        Core::FVector2 mMouseDelta{};

        /**
         * @brief 이번 프레임의 누적 휠 이동량입니다.
         */
        float mMouseWheel{};

        /**
         * @brief 마우스 이동량 계산에 사용할 이전 좌표가 있는지 나타냅니다.
         */
        bool mMouseReady{};

        /**
         * @brief 창의 입력 포커스 상태입니다.
         */
        bool mFocused{};

        /**
         * @brief 이번 프레임의 입력 포커스 상실 여부입니다.
         */
        bool mFocusLost{};

        /**
         * @brief 게임 입력 경로의 활성 요청 상태입니다.
         */
        bool mGameEnabled{};

        /**
         * @brief 입력 경로 전환 프레임의 게임 입력 차단 여부입니다.
         */
        bool mGameFrameBlocked{};
    };

}
