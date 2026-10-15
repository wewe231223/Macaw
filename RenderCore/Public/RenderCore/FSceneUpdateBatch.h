/**
 * @file FSceneUpdateBatch.h
 * @brief 렌더 씬에 전달하는 프리미티브·광원 생성, 변환, 제거와 동적 데이터 변경을 정의합니다.
 */
#pragma once
#include "RenderCore/FSceneHandle.h"
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FLightSceneProxy.h"

#include <memory>
#include <optional>

/**
 * @brief 렌더 씬의 객체에 적용할 변경 종류입니다.
 */
enum class ESceneUpdateType : Uint8 {
    /**
     * @brief 새 프록시를 설치하거나 같은 컴포넌트의 기존 프록시를 교체합니다.
     */
    Create,
    /**
     * @brief 기존 프록시의 월드 변환과 경계를 갱신합니다.
     */
    Transform,
    /**
     * @brief 컴포넌트 핸들에 대응하는 렌더 객체를 제거합니다.
     */
    Remove,
    /**
     * @brief 프리미티브의 빌보드 UV와 그룹별 UV 변환 등 동적 데이터를 갱신합니다. 광원에는 별도 처리가 없습니다.
     */
    DynamicData
};

/**
 * @brief 빌보드 텍스처 영역을 갱신할 UV 범위입니다.
 */
struct FBillboardUV {
    /**
     * @brief 갱신할 최소 UV 좌표입니다.
     */
    FVector2 mUvMin{};
    /**
     * @brief 갱신할 최대 UV 좌표입니다.
     */
    FVector2 mUvMax{};
};

/**
 * @brief 컴포넌트별 프리미티브 프록시 변경 명령입니다.
 * @note Create는 mProxy를, Transform은 mTransform을 사용합니다. Transform과 DynamicData는 선택적 UV 변경도 적용할 수 있습니다.
 */
struct FPrimitiveSceneUpdate {
    /**
     * @brief 변경할 프리미티브 컴포넌트의 비소유 핸들입니다.
     */
    FObjectHandle mComponentHandle{};
    /**
     * @brief 프리미티브에 적용할 변경 종류입니다.
     */
    ESceneUpdateType mType{ESceneUpdateType::Remove};
    /**
     * @brief Create에 전달할 프록시의 소유권입니다. 설치 시 렌더 씬으로 이동합니다.
     */
    std::unique_ptr<FPrimitiveSceneProxy> mProxy{};
    /**
     * @brief Transform 명령으로 적용할 월드 행렬과 경계입니다.
     */
    FPrimitiveTransform mTransform{};
    /**
     * @brief 값이 있을 때 적용할 빌보드 UV 범위입니다.
     */
    std::optional<FBillboardUV> mBillboardUV{};
    /**
     * @brief UV 변환 변경 시 기존 목록을 교체할 그룹별 변환 목록입니다.
     */
    TArray<FPrimitiveUVTransform> mUVTransforms{};
    /**
     * @brief UV 변환 목록을 적용할지 여부입니다. true와 빈 목록의 조합은 기존 변환 목록을 비웁니다.
     */
    bool mUVTransformsChanged{};
};

/**
 * @brief 컴포넌트별 광원 프록시 생성·공간 변환·제거 명령입니다.
 */
struct FLightSceneUpdate {
    /**
     * @brief 변경할 광원 컴포넌트의 비소유 핸들입니다.
     */
    FObjectHandle mComponentHandle{};
    /**
     * @brief 광원에 적용할 변경 종류입니다. Create, Transform, Remove를 사용합니다.
     */
    ESceneUpdateType mType{ESceneUpdateType::Remove};
    /**
     * @brief Create에 전달할 광원 프록시의 소유권입니다. 설치 시 렌더 씬으로 이동합니다.
     */
    std::unique_ptr<FLightSceneProxy> mProxy{};
    /**
     * @brief Transform에서 광원 위치와 Forward 방향을 추출할 월드 행렬입니다.
     */
    FMatrix mWorld{};
};

/**
 * @brief 특정 씬 ID와 세대에 전달할 렌더 상태 변경 묶음입니다.
 */
struct FSceneUpdateBatch {
    /**
     * @brief 변경을 적용할 대상 씬의 현재 핸들입니다.
     */
    FSceneHandle mSceneHandle{};
    /**
     * @brief 기존 렌더 씬 데이터를 재설정한 뒤 배치를 전체 상태로 적용할지 여부입니다.
     */
    bool mFullSnapshot{};
    /**
     * @brief 배치의 프리미티브 변경 목록입니다.
     */
    TArray<FPrimitiveSceneUpdate> mPrimitiveUpdates{};
    /**
     * @brief 배치의 광원 변경 목록입니다.
     */
    TArray<FLightSceneUpdate> mLightUpdates{};
};
