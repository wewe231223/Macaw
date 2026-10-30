/**
 * @file Window.h
 * @brief Win32 창과 메시지 루프를 관리합니다.
 * @ingroup Platform
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <Windows.h>
#include <functional>

namespace Platform {

    /**
     * @brief Win32 창 생성, 메시지 펌프와 클라이언트 영역 상태를 관리합니다.
     * @ingroup Platform
     */
    class FWindowsApplication {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FWindowsApplication);

    public:
        /**
         * @brief 처리한 Win32 메시지이면 true를 반환하는 외부 메시지 콜백입니다.
         */
        using FMessageHandler = std::function<bool(HWND, UINT, WPARAM, LPARAM)>;

    public:
        /**
         * @brief FWindowsApplication의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        FWindowsApplication();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~FWindowsApplication();

        /**
         * @brief 고유한 객체 수명을 유지하기 위해 복사 또는 이동 생성을 금지합니다.
         */
        FWindowsApplication(const Platform::FWindowsApplication&) = delete;

        /**
         * @brief 고유한 객체 수명을 유지하기 위해 대입을 금지합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        Platform::FWindowsApplication& operator=(const Platform::FWindowsApplication&) = delete;

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FWindowsApplication);

        /**
         * @brief 지정한 제목과 클라이언트 크기로 Win32 창을 생성합니다.
         * @param[in] Title 창 또는 오류 보고 제목입니다.
         * @param[in] Width 클라이언트 영역 또는 타깃의 가로 픽셀 수입니다.
         * @param[in] Height 클라이언트 영역 또는 타깃의 세로 픽셀 수입니다.
         * @param[in] Visible 창이나 패널을 표시할지 지정합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool Create(const wchar_t* Title, Core::Uint32 Width, Core::Uint32 Height, bool Visible = true);

        /**
         * @brief 대기 중인 Windows 메시지를 처리하고 실행 지속 여부를 반환합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool PumpMessages();

        /**
         * @brief 종속 객체를 역순으로 정리하고 여러 번 호출해도 안전한 종료 상태로 만듭니다.
         */
        void Shutdown();

        /**
         * @brief 기본 창 처리 전에 호출할 외부 메시지 핸들러를 교체합니다.
         * @param[in] Handler UI 백엔드 등 외부 메시지 처리 콜백입니다.
         */
        void SetMessageHandler(FMessageHandler Handler);

        /**
         * @brief 네이티브 Win32 창 핸들을 반환합니다.
         * @return 네이티브 Win32 창 핸들입니다.
         */
        HWND GetWindowHandle() const;

        /**
         * @brief 가로 픽셀 수를 반환합니다.
         * @return 가로 픽셀 수입니다.
         */
        Core::Uint32 GetWidth() const;

        /**
         * @brief 세로 픽셀 수를 반환합니다.
         * @return 세로 픽셀 수입니다.
         */
        Core::Uint32 GetHeight() const;

        /**
         * @brief 창의 클라이언트 영역이 최소화된 크기인지 검사합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool IsMinimized() const;

    private:
        /**
         * @brief 창 메시지를 인스턴스에 전달하고 크기 및 종료 상태를 갱신합니다.
         * @param[in] Window 메시지를 받은 Win32 창입니다.
         * @param[in] Message 보고할 내용 또는 Windows 메시지 번호입니다.
         * @param[in] WParam Windows 메시지의 WPARAM 인수입니다.
         * @param[in] LParam Windows 메시지의 LPARAM 인수입니다.
         * @return 연산을 적용한 새 값입니다.
         */
        static LRESULT CALLBACK WindowProcedure(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief Win32 창 핸들입니다.
         */
        HWND mWindow{};
        ReflectProperty(mWindow, "Window", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 가로 픽셀 수입니다.
         */
        Core::Uint32 mWidth{};
        ReflectProperty(mWidth, "Width", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 세로 픽셀 수입니다.
         */
        Core::Uint32 mHeight{};
        ReflectProperty(mHeight, "Height", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 메시지 루프가 실행 중인지 여부입니다.
         */
        bool mRunning{};
        ReflectProperty(mRunning, "Running", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 외부 UI 백엔드에 메시지를 전달하는 콜백입니다.
         */
        FMessageHandler mMessageHandler{};
        ReflectProperty(mMessageHandler, "MessageHandler", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
