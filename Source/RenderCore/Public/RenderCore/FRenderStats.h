/**
 * @file FRenderStats.h
 * @brief 프레임 드로 호출, 가시 프리미티브, 변환 업로드와 GPU 메모리 통계입니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

/**
 * @brief 프레임 드로 호출, 가시 프리미티브, 변환 업로드와 GPU 메모리 통계입니다.
 * @ingroup RenderCore
 */
struct FRenderStats {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FRenderStats);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FRenderStats);

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 프레임에서 제출한 드로 호출 수입니다.
     */
    Uint32 mDrawCalls{};
    ReflectProperty(mDrawCalls, "DrawCalls", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 프레임에서 렌더링한 프리미티브 수입니다.
     */
    Uint32 mVisiblePrimitives{};
    ReflectProperty(mVisiblePrimitives, "VisiblePrimitives", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 소비 전 누적된 객체 상수 버퍼 갱신 수입니다.
     */
    Uint32 mTransformUpdates{};
    ReflectProperty(mTransformUpdates, "TransformUpdates", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 집계한 GPU 리소스 바이트 수입니다.
     */
    std::size_t mGpuBytes{};
    ReflectProperty(mGpuBytes, "GpuBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mDirectionalLights{};
    ReflectProperty(mDirectionalLights, "DirectionalLights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mPointLights{};
    ReflectProperty(mPointLights, "PointLights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mSpotLights{};
    ReflectProperty(mSpotLights, "SpotLights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mShadowMaps{};
    ReflectProperty(mShadowMaps, "ShadowMaps", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mShadowDrawCalls{};
    ReflectProperty(mShadowDrawCalls, "ShadowDrawCalls", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mShadowDroppedLights{};
    ReflectProperty(mShadowDroppedLights, "ShadowDroppedLights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Uint32 mShadowReducedLights{};
    ReflectProperty(mShadowReducedLights, "ShadowReducedLights", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mShadowAtlasBytes{};
    ReflectProperty(mShadowAtlasBytes, "ShadowAtlasBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    std::size_t mShadowAllocatedBytes{};
    ReflectProperty(mShadowAllocatedBytes, "ShadowAllocatedBytes", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
