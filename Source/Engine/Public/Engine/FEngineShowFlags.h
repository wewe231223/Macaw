/**
 * @file FEngineShowFlags.h
 * @brief 뷰포트별 장면·보조 표시 필터를 정의합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"

/** @brief 뷰포트마다 독립적으로 적용하는 장면 및 보조 표시 필터입니다. */
struct FEngineShowFlags {
public:
    ReflectTypes(FEngineShowFlags);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectStruct(FEngineShowFlags);

private:
    ReflectMembers();

public:
    /**
     * @brief 모든 장면 프리미티브의 표시 여부입니다.
     */
    bool mLightIcons{true};
    ReflectProperty(mLightIcons, "LightIcons", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mShadows{true};
    ReflectProperty(mShadows, "Shadows", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mLightVisualizers{true};
    ReflectProperty(mLightVisualizers, "LightVisualizers", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mPrimitives{true};
    ReflectProperty(mPrimitives, "Primitives", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 스프라이트와 Billboard의 표시 여부입니다.
     */
    bool mSprites{true};
    ReflectProperty(mSprites, "Sprites", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 장면 텍스트와 에디터 이름 표식의 표시 여부입니다.
     */
    bool mText{true};
    ReflectProperty(mText, "Text", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 AABB 표시 여부입니다.
     */
    bool mBounds{};
    ReflectProperty(mBounds, "Bounds", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 그리드 표시 여부입니다.
     */
    bool mGrid{true};
    ReflectProperty(mGrid, "Grid", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 월드 축 표시 여부입니다.
     */
    bool mAxes{true};
    ReflectProperty(mAxes, "Axes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
