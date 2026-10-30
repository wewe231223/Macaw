/**
 * @file FGizmoSettings.h
 * @brief 뷰포트 기즈모의 스냅 간격과 화면 크기 설정에 대한 선언입니다.
 * @ingroup Editor
 */

#pragma once

#include "Core/ReflectionMacros.h"

namespace Editor {

    /**
     * @brief 뷰포트 클라이언트의 공통 검증 경로에서 변경할 기즈모 수치 설정을 식별합니다.
     * @ingroup Editor
     */
    enum class EGizmoSetting {
        /**
         * @brief 이동 스냅의 양수 거리 간격이며 월드 단위를 사용합니다.
         */
        TranslationStep,
        /**
         * @brief 회전 스냅의 양수 각도 간격이며 도 단위를 사용합니다.
         */
        RotationStep,
        /**
         * @brief 스케일 변화량 스냅의 양수 간격입니다.
         */
        ScaleStep,
        /**
         * @brief 화면에서 유지할 기즈모 크기 배율이며 0.5부터 2.0까지로 제한합니다.
         */
        Size
    };

    /**
     * @brief 기즈모 스냅의 활성 상태·간격과 화면 크기 설정입니다.
     * @ingroup Editor
     */
    struct FGizmoSettings {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FGizmoSettings);

    public:
        /**
         * @brief 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 정보입니다.
         */
        ReflectStruct(FGizmoSettings);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 이동 거리를 설정한 간격으로 양자화할지 지정합니다.
         */
        bool mTranslationSnap{};
        ReflectProperty(mTranslationSnap, "TranslationSnap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 회전 각도를 설정한 각도 간격으로 양자화할지 지정합니다.
         */
        bool mRotationSnap{};
        ReflectProperty(mRotationSnap, "RotationSnap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 축 스케일 증가량이나 균일 배율 증가량을 단계로 양자화할지 지정합니다.
         */
        bool mScaleSnap{};
        ReflectProperty(mScaleSnap, "ScaleSnap", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 이동 스냅의 월드 거리 간격이며 유한한 양수일 때 적용합니다.
         */
        float mTranslationStep{1.0f};
        ReflectProperty(mTranslationStep, "TranslationStep", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 회전 스냅의 도 단위 간격이며 유한한 양수일 때 적용합니다.
         */
        float mRotationStep{15.0f};
        ReflectProperty(mRotationStep, "RotationStep", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 스케일 스냅의 증가량 간격이며 유한한 양수일 때 적용합니다.
         */
        float mScaleStep{0.1f};
        ReflectProperty(mScaleStep, "ScaleStep", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 화면에서 유지할 기즈모 크기의 배율이며 0.5에서 2 사이를 사용합니다.
         */
        float mSize{1.0f};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
