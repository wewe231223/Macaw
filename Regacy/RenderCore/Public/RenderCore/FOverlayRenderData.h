/**
 * @file FOverlayRenderData.h
 * @brief 선택 윤곽선, 가이드, 기즈모, 화면 텍스트와 방향 축의 오버레이 데이터를 정의합니다.
 */

#pragma once

#include <bitset>
#include "RenderCore/FRenderData.h"
#include "RenderCore/FLineRenderData.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

/**
 * @brief 개별 활성 상태를 제어할 수 있는 오버레이 패스입니다.
 */
enum class EOverlayPass : Uint8 {
    /**
     * @brief 선택한 메시의 윤곽선 패스입니다.
     */
    SelectionOutline,
    /**
     * @brief 격자와 디버그 가이드 선 패스입니다.
     */
    Guides,
    /**
     * @brief 변환 기즈모의 메시와 선 패스입니다.
     */
    Gizmo,
    /**
     * @brief 월드 기준점에 배치한 화면 텍스트 패스입니다.
     */
    Text,
    /**
     * @brief 뷰 방향을 나타내는 화면 방향 축 패스입니다.
     */
    OrientationAxis,
    /**
     * @brief 유효한 패스 수와 활성 비트 집합의 크기입니다.
     */
    Count
};

/**
 * @brief 월드 기준점을 화면에 투영해 배치할 텍스트 레이블 데이터입니다.
 */
struct FOverlayTextDrawData {
    /**
     * @brief 텍스트의 화면 위치를 계산할 월드 공간 기준점입니다.
     */
    FVector3 mWorldAnchor{};

    /**
     * @brief 기준점 중심의 경계 상자 반크기입니다. 화면상 경계 위쪽에 텍스트를 배치할 때 사용합니다.
     */
    FVector3 mWorldBoundsExtent{};

    /**
     * @brief 투영한 화면 위치에 더할 픽셀 오프셋입니다. X는 오른쪽, Y는 아래쪽이 양수입니다.
     */
    FVector2 mScreenOffset{0.0f, -8.0f};

    /**
     * @brief 글꼴 에셋 핸들입니다. 무효 값의 기본 글꼴 대체 여부는 렌더러가 결정합니다.
     */
    FAssetHandle mFontHandle{};

    /**
     * @brief 텍스트의 RGBA 색상입니다.
     */
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

    /**
     * @brief 그릴 UTF-8 텍스트입니다.
     */
    FString mText{};

    /**
     * @brief 픽셀 단위로 요청할 문자 높이입니다.
     */
    float mPixelHeight{24.0f};

    /**
     * @brief 글리프 사이에 추가할 픽셀 간격입니다.
     */
    float mLetterSpacing{};

    /**
     * @brief 줄 사이에 추가할 픽셀 간격입니다.
     */
    float mLineSpacing{};
};

/**
 * @brief 선택 윤곽선 또는 기즈모에 제출할 메시 인스턴스 데이터입니다.
 */
struct FOverlayMeshDrawData {
    /**
     * @brief 메시의 월드 배치 행렬입니다.
     */
    FMatrix mWorld{};

    /**
     * @brief 메시·머티리얼·파이프라인 에셋 식별자입니다.
     */
    FMeshSceneData mMesh{};

    /**
     * @brief 메시의 월드 공간 구 경계입니다.
     */
    DirectX::BoundingSphere mWorldSphereBounds{};
};

/**
 * @brief 한 뷰에 제출할 오버레이 목록과 패스 활성 설정을 묶은 데이터입니다.
 */
struct FOverlayRenderData {
    /**
     * @brief 수집한 메시·선·텍스트 목록과 격자 페이드 파라미터를 비웁니다.
     * @note 패스 활성 비트와 방향 축 크기는 유지합니다.
     */
    void Clear();

    /**
     * @brief 지정한 오버레이 패스가 켜져 있는지 검사합니다.
     * @param Pass 검사할 패스입니다.
     * @return 유효한 패스이고 활성 비트가 켜져 있으면 true입니다. Count나 범위 밖 값이면 false입니다.
     */
    bool IsPassEnabled(EOverlayPass Pass) const;

    /**
     * @brief 유효한 오버레이 패스의 활성 비트를 설정합니다.
     * @param Pass 변경할 패스입니다.
     * @param Enabled 적용할 활성 상태입니다.
     * @note Count나 범위 밖 값은 무시합니다.
     */
    void SetPassEnabled(EOverlayPass Pass, bool Enabled);

    /**
     * @brief 패스별 활성 비트입니다. 기본 상태는 모든 유효 패스를 활성화합니다.
     */
    std::bitset<static_cast<std::size_t>(EOverlayPass::Count)> mPasses{(1ull << static_cast<std::size_t>(EOverlayPass::Count)) - 1};

    /**
     * @brief 화면 방향 축 영역의 픽셀 크기입니다. 0 이하이면 렌더러가 뷰 크기에 따라 결정합니다.
     */
    float mOrientationAxisSize{};

    /**
     * @brief 선택 윤곽선을 표시할 메시 목록입니다.
     */
    TArray<FOverlayMeshDrawData> mSelectionMeshes{};

    /**
     * @brief 기즈모에 사용할 메시 목록입니다.
     */
    TArray<FOverlayMeshDrawData> mGizmoMeshes{};

    /**
     * @brief 기즈모에 사용할 선분 목록입니다.
     */
    FLineRenderData mGizmoLines{};

    /**
     * @brief 화면 텍스트 레이블 목록입니다.
     */
    TArray<FOverlayTextDrawData> mTextDraws{};

    /**
     * @brief 격자와 디버그 가이드 선분 목록입니다.
     */
    FLineRenderData mGuides{};

    /**
     * @brief 격자 페이드의 월드 XY 중심과 거리 범위입니다. X·Y는 중심, Z는 페이드 시작 거리, W는 종료 거리입니다.
     */
    FVector4 mGridFade{};
};
