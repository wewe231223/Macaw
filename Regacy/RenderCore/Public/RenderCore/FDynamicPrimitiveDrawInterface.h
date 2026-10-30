/**
 * @file FDynamicPrimitiveDrawInterface.h
 * @brief 프록시가 텍스트와 빌보드 그리기 데이터를 제출하는 인터페이스를 정의합니다.
 */

#pragma once
#include "RenderCore/FRenderData.h"

/**
 * @brief 동적 프리미티브 데이터와 현재 월드 변환을 수집하는 추상 인터페이스입니다.
 */
class FDynamicPrimitiveDrawInterface {
public:
    /**
     * @brief 파생 그리기 인터페이스를 기반 포인터로 해제할 수 있게 합니다.
     */
    virtual ~FDynamicPrimitiveDrawInterface();

public:
    /**
     * @brief 텍스트 그리기 데이터를 현재 월드 변환과 함께 제출합니다.
     * @param Text 글꼴·파이프라인·색상과 글리프 레코드 목록입니다.
     * @param World 이번 그리기에 적용할 현재 월드 행렬입니다.
     */
    virtual void DrawText(const FTextDrawData& Text, const FMatrix& World) = 0;

    /**
     * @brief 빌보드 그리기 데이터를 현재 월드 변환과 함께 제출합니다.
     * @param Billboard 텍스처·파이프라인·크기·색상과 UV 범위입니다.
     * @param World 이번 그리기에 적용할 현재 월드 행렬입니다.
     */
    virtual void DrawBillboard(const FBillboardDrawData& Billboard, const FMatrix& World) = 0;
};
