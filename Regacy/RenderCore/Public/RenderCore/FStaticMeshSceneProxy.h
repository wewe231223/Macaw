/**
 * @file FStaticMeshSceneProxy.h
 * @brief 메시 에셋 정보를 정적 LOD별 그리기 배치로 전달하는 프록시를 정의합니다.
 */

#pragma once

#include "RenderCore/FPrimitiveSceneProxy.h"

/**
 * @brief 메시·머티리얼·파이프라인 에셋을 보관하는 정적 메시 프록시입니다.
 */
class FStaticMeshSceneProxy final : public FPrimitiveSceneProxy {
public:
    /**
     * @brief 원본 핸들, 변환과 메시 에셋 정보를 복사합니다.
     * @param ComponentHandle 원본 컴포넌트의 비소유 핸들입니다.
     * @param OwnerHandle 원본 소유 객체의 비소유 핸들입니다.
     * @param Transform 월드 행렬과 공간 경계입니다.
     * @param MeshData 사용할 에셋 식별자 묶음입니다.
     */
    FStaticMeshSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, const FMeshSceneData& MeshData);

    /**
     * @brief 정적 메시 프록시를 해제합니다. 참조한 에셋을 직접 삭제하지 않습니다.
     */
    ~FStaticMeshSceneProxy() override = default;

public:
    /**
     * @brief 내부 메시 에셋 정보를 반환합니다.
     * @return 프록시 수명 동안 유효한 비소유 상수 포인터입니다.
     */
    const FMeshSceneData* GetMeshData() const override;

    /**
     * @brief 모든 사용 가능한 LOD의 구간을 조회하고 비어 있지 않은 배치를 전달합니다.
     * @param DrawInterface 메시 LOD 조회와 정적 배치 수집 인터페이스입니다.
     * @note 이 함수에서 화면상의 최종 LOD를 선택하지 않습니다.
     */
    void DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const override;

private:
    /**
     * @brief 정적 메시의 에셋 식별자 복사본입니다.
     */
    FMeshSceneData mMeshData{};
};
