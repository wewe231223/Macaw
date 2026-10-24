#pragma once

#include "Core/ReflectionMacros.h"

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

struct FGizmoSettings {
public:
    ReflectTypes(FGizmoSettings);

public:
    ReflectStruct(FGizmoSettings);

private:
    ReflectMembers();

public:
    bool mTranslationSnap{};
    ReflectProperty(mTranslationSnap, "TranslationSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mRotationSnap{};
    ReflectProperty(mRotationSnap, "RotationSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mScaleSnap{};
    ReflectProperty(mScaleSnap, "ScaleSnap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mTranslationStep{1.0f};
    ReflectProperty(mTranslationStep, "TranslationStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mRotationStep{15.0f};
    ReflectProperty(mRotationStep, "RotationStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mScaleStep{0.1f};
    ReflectProperty(mScaleStep, "ScaleStep", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mSize{1.0f};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
