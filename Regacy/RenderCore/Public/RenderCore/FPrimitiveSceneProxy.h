/**
 * @file FPrimitiveSceneProxy.h
 * @brief 렌더링용 프리미티브의 월드 경계, 에셋 참조와 UV 상태를 보관하는 프록시를 정의합니다.
 */
#pragma once

#include "Core/CoreMinimal.h"
#include "Core/Base/FAssetHandle.h"
#include "CoreUObject/FObjectHandle.h"
#include "RenderCore/FUVTransform.h"

#include <span>

class FStaticPrimitiveDrawInterface;
class FDynamicPrimitiveDrawInterface;

/**
 * @brief 프리미티브의 월드 행렬과 컬링·공간 조회용 경계 묶음입니다.
 * @note 행렬과 경계를 독립적으로 저장하므로 호출자가 서로 일치하는 값을 제공해야 합니다.
 */
struct FPrimitiveTransform {
    /**
     * @brief 로컬 공간을 월드 공간으로 변환하는 행렬입니다.
     */
    FMatrix mWorld{};
    /**
     * @brief 월드 공간의 구 경계입니다.
     */
    DirectX::BoundingSphere mWorldSphereBounds{};
    /**
     * @brief 월드 공간의 방향성 경계 상자입니다.
     */
    DirectX::BoundingOrientedBox mWorldOBB{};
    /**
     * @brief 월드 공간의 축 정렬 경계 상자입니다.
     */
    DirectX::BoundingBox mWorldAABB{};
};

/**
 * @brief 메시 프리미티브가 사용할 메시·머티리얼·파이프라인 에셋 식별자입니다.
 */
struct FMeshMaterialBinding {
    FName mSlot{};
    FAssetHandle mMaterial{};
};

struct FMeshSceneData {
    FAssetHandle mMeshHandle{};
    TArray<FMeshMaterialBinding> mMaterialOverrides{};
};

/**
 * @brief 원본 객체의 핸들과 렌더 상태를 보관하고 정적·동적 그리기 확장 지점을 제공하는 프록시입니다.
 * @note 원본 컴포넌트와 소유 객체의 수명을 연장하지 않습니다. 기반 구현은 그리기 명령을 생성하지 않습니다.
 */
class FPrimitiveSceneProxy {
public:
    /**
     * @brief 원본 식별 핸들과 월드 변환·경계를 복사해 초기화합니다.
     * @param ComponentHandle 원본 컴포넌트의 비소유 핸들입니다.
     * @param OwnerHandle 원본 소유 객체의 비소유 핸들입니다.
     * @param Transform 일치하는 월드 행렬과 경계입니다.
     */
    FPrimitiveSceneProxy(FObjectHandle ComponentHandle, FObjectHandle OwnerHandle, const FPrimitiveTransform& Transform);
    /**
     * @brief 파생 프록시와 내부 UV 변환 목록을 해제합니다.
     */
    virtual ~FPrimitiveSceneProxy();
    /**
     * @brief 프록시 렌더 상태의 암시적 복사 생성을 금지합니다.
     */
    FPrimitiveSceneProxy(const FPrimitiveSceneProxy&) = delete;
    /**
     * @brief 프록시 렌더 상태의 복사 대입을 금지합니다.
     */
    FPrimitiveSceneProxy& operator=(const FPrimitiveSceneProxy&) = delete;
    /**
     * @brief 프록시 객체의 이동 생성을 금지합니다.
     */
    FPrimitiveSceneProxy(FPrimitiveSceneProxy&&) = delete;
    /**
     * @brief 프록시 객체의 이동 대입을 금지합니다.
     */
    FPrimitiveSceneProxy& operator=(FPrimitiveSceneProxy&&) = delete;

public:
    /**
     * @brief 원본 컴포넌트의 저장된 핸들을 반환합니다.
     * @return 생존 여부를 추가 검사하지 않은 컴포넌트 핸들입니다.
     */
    FObjectHandle GetComponentHandle() const;
    /**
     * @brief 원본 소유 객체의 저장된 핸들을 반환합니다.
     * @return 생존 여부를 추가 검사하지 않은 소유 객체 핸들입니다.
     */
    FObjectHandle GetOwnerHandle() const;
    /**
     * @brief 현재 월드 변환과 경계를 반환합니다.
     * @return 내부 변환 데이터의 상수 참조입니다.
     */
    const FPrimitiveTransform& GetTransform() const;
    /**
     * @brief 메시 프리미티브의 에셋 정보를 조회하는 확장 지점입니다.
     * @return 메시 정보의 비소유 포인터입니다. 기반 구현은 nullptr입니다.
     */
    virtual const FMeshSceneData* GetMeshData() const;
    /**
     * @brief 정적 메시 배치를 수집하는 확장 지점입니다.
     * @param DrawInterface 정적 메시 조회와 배치 전달 인터페이스입니다.
     * @note 기반 구현은 아무 명령도 전달하지 않습니다.
     */
    virtual void DrawStaticElements(FStaticPrimitiveDrawInterface& DrawInterface) const;
    /**
     * @brief 텍스트·빌보드 등의 동적 그리기 데이터를 수집하는 확장 지점입니다.
     * @param DrawInterface 동적 그리기 전달 인터페이스입니다.
     * @note 기반 구현은 아무 명령도 전달하지 않습니다.
     */
    virtual void GetDynamicMeshElements(FDynamicPrimitiveDrawInterface& DrawInterface) const;
    /**
     * @brief 저장된 월드 변환과 경계를 함께 교체합니다.
     * @param Transform 적용할 월드 행렬과 경계입니다. 경계를 자동 재계산하지 않습니다.
     */
    void SetTransform(const FPrimitiveTransform& Transform);
    /**
     * @brief 빌보드 UV 범위 갱신의 확장 지점입니다.
     * @param UVMin 최소 UV 좌표입니다.
     * @param UVMax 최대 UV 좌표입니다.
     * @note 기반 구현은 전달한 값을 저장하지 않습니다.
     */
    virtual void SetBillboardUV(const FVector2& UVMin, const FVector2& UVMax);
    /**
     * @brief 그룹별 UV 변환 목록을 복사해 교체합니다.
     * @param Transforms 복사할 목록입니다. 빈 범위이면 기존 목록을 비웁니다.
     */
    void SetUVTransforms(std::span<const FPrimitiveUVTransform> Transforms);
    /**
     * @brief 그룹 이름에 대응하는 UV 변환을 선택합니다.
     * @param MaterialSlot 요청할 머티리얼 그룹입니다. 기본값은 None입니다.
     * @return 첫 정확한 이름 일치의 변환입니다. 없으면 마지막 None 항목, 그마저 없으면 항등 변환입니다.
     */
    FUVTransform GetUVTransform(FName MaterialSlot = {}) const;

private:
    /**
     * @brief 원본 컴포넌트의 비소유 핸들입니다.
     */
    FObjectHandle mComponentHandle{};
    /**
     * @brief 원본 소유 객체의 비소유 핸들입니다.
     */
    FObjectHandle mOwnerHandle{};
    /**
     * @brief 현재 월드 행렬과 공간 경계의 복사본입니다.
     */
    FPrimitiveTransform mTransform{};
    /**
     * @brief 원본 순서로 보관한 그룹별 UV 변환 목록입니다.
     */
    TArray<FPrimitiveUVTransform> mUVTransforms{};
};
