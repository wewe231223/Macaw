/**
 * @file UBoxComponent.h
 * @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UShapeComponent.h"

/** @brief 로컬 반길이로 정의한 상자 충돌 컴포넌트입니다. */
class UBoxComponent : public UShapeComponent {
public:
    ReflectTypes(UBoxComponent, UShapeComponent);

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UBoxComponent);
    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FCollisionShape GetCollisionShape() const override;
    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FBox CalcLocalBounds() const override;
    /**
     * @brief 상자의 로컬 축별 반길이를 설정합니다.
     * @param[in] InExtent 상자 각 축의 반길이입니다.
     */
    void SetBoxExtent(const FVector& InExtent);
    /**
     * @brief 월드 스케일을 반영한 상자 축별 반길이를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FVector GetScaledBoxExtent() const;

    /**
     * @brief 상자의 로컬 축별 반길이를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector& GetBoxExtent() const;

private:
    ReflectMembers();

private:
    /**
     * @brief 상자의 로컬 축별 반길이입니다.
     */
    FVector mBoxExtent{0.5f, 0.5f, 0.5f};
    ReflectAccessor(mBoxExtent, GetBoxExtent, SetBoxExtent, "BoxExtent", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
