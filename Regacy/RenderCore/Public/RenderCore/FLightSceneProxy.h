/**
 * @file FLightSceneProxy.h
 * @brief 광원 컴포넌트 핸들과 갱신 가능한 셰이더 파라미터를 보관합니다.
 */

#pragma once
#include "RenderCore/FRenderData.h"
#include "CoreUObject/FObjectHandle.h"

/**
 * @brief 원본 광원 객체의 수명과 별도로 광원 셰이더 상태를 복사해 보관하는 프록시입니다.
 */
class FLightSceneProxy {
public:
    /**
     * @brief 광원 컴포넌트 핸들과 셰이더 파라미터를 저장합니다.
     * @param ComponentHandle 원본 광원 컴포넌트의 비소유 핸들입니다.
     * @param Parameters 복사할 광원 셰이더 데이터입니다.
     */
    FLightSceneProxy(FObjectHandle ComponentHandle, const FLightShaderParameters& Parameters);

public:
    /**
     * @brief 원본 광원 컴포넌트의 핸들을 반환합니다.
     * @return 생존 여부를 추가 검사하지 않은 비소유 핸들입니다.
     */
    FObjectHandle GetComponentHandle() const;

    /**
     * @brief 현재 광원 셰이더 데이터를 반환합니다.
     * @return 내부 파라미터의 상수 참조입니다.
     */
    const FLightShaderParameters& GetShaderParameters() const;

    /**
     * @brief 월드 행렬의 이동과 Forward 축으로 광원 위치·방향을 갱신합니다.
     * @param World 위치와 방향을 추출할 월드 행렬입니다.
     * @note 다른 광원 파라미터는 유지하며 추출한 방향을 별도로 정규화하지 않습니다.
     */
    void SetTransform(const FMatrix& World);

private:
    /**
     * @brief 원본 광원 컴포넌트의 비소유 핸들입니다.
     */
    FObjectHandle mComponentHandle{};

    /**
     * @brief 현재 광원 셰이더 파라미터의 복사본입니다.
     */
    FLightShaderParameters mParameters{};
};
