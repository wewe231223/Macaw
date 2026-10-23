/**
 * @file FBillboardSceneProxy.h
 * @brief 빌보드 상태와 UV 범위를 동적 그리기 인터페이스에 전달하는 프록시를 정의합니다.
 */
#pragma once
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FRenderData.h"

/**
 * @brief 텍스처 사각형 데이터와 갱신 가능한 UV 범위를 보관하는 프록시입니다.
 */
class FBillboardSceneProxy final : public FPrimitiveSceneProxy {
public:
    /**
     * @brief 원본 식별·공간 정보를 복사하고 빌보드 데이터를 이동해 보관합니다.
     * @param ComponentHandle 원본 컴포넌트의 비소유 핸들입니다.
     * @param OwnerHandle 원본 소유 객체의 비소유 핸들입니다.
     * @param Transform 월드 행렬과 공간 경계입니다.
     * @param Data 보관할 빌보드 그리기 데이터입니다.
     */
    FBillboardSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FBillboardDrawData Data);
    /**
     * @brief 빌보드 프록시를 해제합니다. 참조한 에셋을 직접 삭제하지 않습니다.
     */
    ~FBillboardSceneProxy() override = default;

public:
    /**
     * @brief 기본 UV 변환을 최소·최대 UV에 적용하고 빌보드 그리기를 전달합니다.
     * @param DrawInterface 빌보드 데이터를 수집할 인터페이스입니다.
     * @note 저장된 원본 UV는 유지하며 현재 프록시의 월드 행렬을 별도 인수로 전달합니다.
     */
    void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const override;
    /**
     * @brief UV 변환 적용 전의 빌보드 UV 범위를 교체합니다.
     * @param UVMin 최소 UV 좌표입니다.
     * @param UVMax 최대 UV 좌표입니다.
     */
    void SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax) override;

private:
    /**
     * @brief 빌보드 그리기에 사용할 원본 데이터입니다.
     */
    FBillboardDrawData mData{};
};
