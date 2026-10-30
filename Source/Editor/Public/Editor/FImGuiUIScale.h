/**
 * @file FImGuiUIScale.h
 * @brief 창의 DPI와 사용자 설정에 따라 ImGui 스타일과 글꼴 배율을 적용합니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include <imgui.h>

namespace Platform {
    class FWindowsApplication;
}

namespace Editor {

    /**
     * @brief 창의 DPI와 사용자 설정에 따라 ImGui 스타일과 글꼴 배율을 적용합니다.
     * @ingroup Editor
     */
    class FImGuiUIScale {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FImGuiUIScale);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FImGuiUIScale);

        /**
         * @brief 현재 ImGui 스타일과 기본 글꼴 크기를 배율 적용의 기준으로 저장합니다.
         * @param[in] FontSize 배율 적용 전의 기준 글꼴 크기입니다.
         */
        void Initialize(float FontSize);

        /**
         * @brief 현재 창의 DPI·크기와 사용자 설정에 맞춰 ImGui 배율을 갱신합니다.
         * @param[in] Platform 창 크기와 DPI를 조회할 플랫폼 객체입니다.
         */
        void Update(const Platform::FWindowsApplication& Platform);

        /**
         * @brief 자동·사용자 UI 배율을 선택하는 메뉴를 그립니다.
         * @return 메뉴에서 사용자 배율 설정을 변경했으면 true, 변경하지 않았으면 false입니다.
         */
        bool DrawMenu();

        /**
         * @brief 10~32 범위의 폰트 크기로 배율 적용 전 스타일을 교체하고 다음 프레임의 배율 재적용을 예약합니다.
         * @param[in] Style 읽거나 갱신할 배율 적용 전 ImGui 스타일입니다.
         * @param[in] FontSize 10~32 범위의 유한한 기본 폰트 크기입니다.
         */
        void SetBaseStyle(const ImGuiStyle& Style, float FontSize);
        /**
         * @brief 자동 배율 0 또는 0.75~3.0 범위의 유한한 사용자 UI 배율을 설정합니다.
         * @param[in] Scale 자동 배율 0 또는 유한한 0.75~3.0 사용자 배율입니다.
         * @return 유효한 값으로 기존 배율이 바뀌면 true이며 같은 값이거나 유효하지 않으면 false입니다.
         */
        bool SetUserScale(float Scale);
        /**
         * @brief 사용자가 선택한 UI 배율을 조회합니다.
         * @return 지정한 배율이며 0이면 DPI·창 크기에 따른 자동 배율입니다.
         */
        float GetUserScale() const;

        /**
         * @brief 현재 ImGui 스타일에 적용한 최종 배율을 반환합니다.
         * @return DPI와 사용자 설정이 반영된 현재 배율입니다.
         */
        float GetScale() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 배율을 적용하기 전의 기준 ImGui 스타일입니다.
         */
        ImGuiStyle mBaseStyle{};
        ReflectProperty(mBaseStyle, "BaseStyle", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 현재 ImGui 스타일에 적용한 최종 배율입니다.
         */
        float mAppliedScale{};
        ReflectProperty(mAppliedScale, "AppliedScale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief DPI 배율에 곱할 사용자 배율이며 0이면 자동 크기를 선택합니다.
         */
        float mUserScale{};
        ReflectProperty(mUserScale, "UserScale", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
