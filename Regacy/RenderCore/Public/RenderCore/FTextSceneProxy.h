/**
 * @file FTextSceneProxy.h
 * @brief 저장한 글리프 데이터를 동적 그리기 인터페이스에 전달하는 텍스트 프록시를 정의합니다.
 */

#pragma once
#include "RenderCore/FPrimitiveSceneProxy.h"
#include "RenderCore/FRenderData.h"

/**
 * @brief 글꼴·색상과 글리프 레코드 목록을 보관하는 텍스트 프록시입니다.
 */
class FTextSceneProxy final : public FPrimitiveSceneProxy {
public:
    /**
     * @brief 원본 식별·공간 정보를 복사하고 텍스트 데이터를 이동해 보관합니다.
     * @param ComponentHandle 원본 컴포넌트의 비소유 핸들입니다.
     * @param OwnerHandle 원본 소유 객체의 비소유 핸들입니다.
     * @param Transform 월드 행렬과 공간 경계입니다.
     * @param Data 보관할 텍스트 그리기 데이터입니다.
     */
    FTextSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform, FTextDrawData Data);

    /**
     * @brief 텍스트 프록시와 소유한 글리프 목록을 해제합니다.
     */
    ~FTextSceneProxy() override = default;

public:
    /**
     * @brief 보관한 텍스트 데이터와 현재 프록시 월드 행렬을 전달합니다.
     * @param DrawInterface 텍스트 데이터를 수집할 인터페이스입니다.
     */
    void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const override;

private:
    /**
     * @brief 글리프 목록을 포함해 프록시가 보관하는 텍스트 데이터입니다.
     */
    FTextDrawData mData{};
};
