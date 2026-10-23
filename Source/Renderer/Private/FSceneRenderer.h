/**
 * @file FSceneRenderer.h
 * @brief 뷰 프러스텀으로 프리미티브를 선별하여 장면 드로 호출을 제출합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "FSceneDrawContext.h"
#include "RenderCore/FRenderStats.h"

struct FSceneViewFamily;
class FScene;
struct FBox;

/**
 * @brief 뷰 프러스텀으로 프리미티브를 선별하여 장면 드로 호출을 제출합니다.
 * @ingroup Renderer
 */
class FSceneRenderer {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FSceneRenderer);

public:
    /**
     * @brief FSceneRenderer의 초기 상태와 전달받은 의존성을 구성합니다.
     * @param[in] Family 장면, 출력 타깃과 카메라를 묶은 렌더 요청입니다.
     * @param[in] Scene 연결하거나 렌더링할 장면입니다.
     * @param[in] Context 월드 컨텍스트 또는 Direct3D 11 컨텍스트입니다.
     * @param[in,out] Stats 렌더링 결과를 누적할 통계입니다.
     */
    FSceneRenderer(const FSceneViewFamily& Family, const FScene* Scene, const FSceneDrawContext& Context, FRenderStats& Stats);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FSceneRenderer);

    /**
     * @brief 가시 프리미티브를 선별하여 드로 호출을 제출하고 통계를 갱신합니다.
     */
    void Render();

private:
    /**
     * @brief 경계 상자가 뷰 프러스텀과 겹치는지 검사합니다.
     * @param[in] Bounds 프리미티브의 월드 또는 로컬 경계 상자입니다.
     * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
     */
    bool IsVisible(const FBox& Bounds) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 렌더링할 뷰 패밀리의 비소유 주소입니다.
     */
    const FSceneViewFamily* mFamily{};
    ReflectProperty(mFamily, "Family", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 연결된 렌더 장면입니다.
     */
    const FScene* mScene{};
    ReflectProperty(mScene, "Scene", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 드로 제출에 필요한 파이프라인 리소스 묶음입니다.
     */
    FSceneDrawContext mDrawContext{};
    ReflectProperty(mDrawContext, "DrawContext", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 프레임 렌더링 통계입니다.
     */
    FRenderStats* mStats{};
    ReflectProperty(mStats, "Stats", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
