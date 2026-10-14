/**
 * @file FTransform.h
 * @brief 위치, 회전, 스케일과 절대 변환 설정 및 행렬 캐시를 가진 변환 타입을 정의한다.
 */
#pragma once

#include "Core/CoreMinimal.h"
#include "Math/FMath.h"
#include "Core/Archive/FArchive.h"

/**
 * @brief 위치, 쿼터니언 회전, 오일러 회전과 스케일을 보관하고 부모 변환과의 결합을 지원한다.
 * @details 실제 상태 변경 시 리비전을 증가시키며 필요한 행렬 캐시를 무효화한다. 행렬은 조회 시 다시 계산한다.
 * @details 절대 위치, 회전, 스케일 설정은 부모와의 결합 및 상대 변환 계산에 사용한다. 자체 행렬은 저장된 위치, 회전, 스케일로 구성한다.
 */
struct FTransform {
public:
    /** @brief 위치와 오일러 회전이 0이고, 회전은 단위 쿼터니언이며, 스케일이 1인 기본 변환을 생성한다. */
    FTransform() = default;

    /**
     * @brief 위치, 오일러 회전 및 스케일로 변환을 생성한다.
     * @param InPosition 저장할 위치.
     * @param InRotation 도 단위의 Pitch, Yaw, Roll 회전. 쿼터니언도 함께 계산한다.
     * @param InScale 각 축의 스케일.
     */
    FTransform(const FVector3& InPosition, const FRotator& InRotation, const FVector3& InScale);

    /**
     * @brief 위치, 쿼터니언 회전 및 스케일로 변환을 생성한다.
     * @param InPosition 저장할 위치.
     * @param InRotation 저장할 회전. 정규화한 뒤 오일러 회전도 계산한다.
     * @param InScale 각 축의 스케일.
     */
    FTransform(const FVector3& InPosition, const FQuat& InRotation, const FVector3& InScale);

    /**
     * @brief 변환 상태, 리비전 및 행렬 캐시를 복사하여 새 변환을 생성한다.
     * @param Other 복사할 변환.
     */
    FTransform(const FTransform& Other) = default;
    /**
     * @brief 변환 성분과 절대 변환 설정을 복사하여 현재 객체를 갱신한다.
     * @param Other 복사할 변환.
     * @return 현재 객체의 참조.
     * @details 자기 자신이거나 상태가 같으면 변경하지 않는다. 상태가 다르면 현재 객체의 리비전을 1 증가시키고 성분 변경에 따라 행렬 캐시를 무효화한다. 원본의 리비전과 캐시는 복사하지 않는다.
     */
    FTransform& operator=(const FTransform& Other);
    /**
     * @brief 변환 상태, 리비전 및 행렬 캐시를 이동하여 새 변환을 생성한다.
     * @param Other 이동할 변환.
     */
    FTransform(FTransform&& Other) noexcept = default;
    /**
     * @brief 복사 할당과 동일한 방식으로 현재 변환을 갱신한다.
     * @param Other 갱신에 사용할 변환.
     * @return 현재 객체의 참조.
     * @details 복사 할당에 처리를 위임하므로 원본을 비우지 않으며 상태 변경 시 현재 객체의 리비전을 증가시킨다.
     */
    FTransform& operator=(FTransform&& Other) noexcept;

public:
    /**
     * @brief 현재 객체의 상태 변경 리비전을 조회한다.
     * @return 저장된 리비전 값.
     * @details 행렬 캐시를 다시 계산하는 것만으로는 리비전이 변경되지 않는다.
     */
    Uint64 GetRevision() const;

    /**
     * @brief 저장된 위치를 조회한다.
     * @return 현재 객체가 소유하는 위치의 읽기 전용 참조.
     */
    const FVector3& GetPosition() const;

    /**
     * @brief 저장된 도 단위 오일러 회전을 조회한다.
     * @return 현재 객체가 소유하는 Pitch, Yaw, Roll 회전의 읽기 전용 참조.
     */
    const FRotator& GetRotation() const;

    /**
     * @brief 회전 계산에 사용하는 쿼터니언을 조회한다.
     * @return 현재 객체가 소유하는 회전 쿼터니언의 읽기 전용 참조.
     */
    const FQuat& GetRotationQuaternion() const;

    /**
     * @brief 각 축의 스케일을 조회한다.
     * @return 현재 객체가 소유하는 스케일의 읽기 전용 참조.
     */
    const FVector3& GetScale() const;

    /**
     * @brief 위치에 부모 변환을 적용하지 않도록 설정되어 있는지 확인한다.
     * @return 부모와 결합할 때 자신의 위치를 그대로 유지하면 true.
     */
    bool IsAbsoluteLocation() const;

    /**
     * @brief 부모 회전을 상속하지 않도록 설정되어 있는지 확인한다.
     * @return 부모와 결합할 때 자신의 회전을 그대로 유지하면 true.
     */
    bool IsAbsoluteRotation() const;

    /**
     * @brief 부모 스케일을 상속하지 않도록 설정되어 있는지 확인한다.
     * @return 부모와 결합할 때 자신의 스케일을 그대로 유지하면 true.
     */
    bool IsAbsoluteScale() const;

    /**
     * @brief 위치를 갱신한다.
     * @param InPosition 새 위치.
     * @details 기존 값과 다르면 모든 행렬 캐시를 무효화하고 리비전을 1 증가시킨다.
     */
    void SetPosition(const FVector3& InPosition);

    /**
     * @brief 도 단위 오일러 회전과 대응하는 쿼터니언을 갱신한다.
     * @param InRotation 새 Pitch, Yaw, Roll 회전.
     * @details 오일러 성분이 기존 값과 다르면 쿼터니언을 다시 계산하고 모든 행렬 캐시를 무효화하며 리비전을 1 증가시킨다.
     */
    void SetRotation(const FRotator& InRotation);
    /**
     * @brief 쿼터니언 회전을 정규화하여 저장하고 오일러 회전도 갱신한다.
     * @param InRotation 새 회전 쿼터니언.
     * @details 입력 또는 정규화한 입력의 성분이 기존 쿼터니언과 같으면 변경하지 않는다. 변경 시 모든 행렬 캐시를 무효화하고 리비전을 1 증가시킨다.
     */
    void SetRotation(const FQuat& InRotation);

    /**
     * @brief 각 축의 스케일을 갱신한다.
     * @param InScale 새 스케일.
     * @details 기존 값과 다르면 스케일 포함 행렬과 그 역행렬 캐시를 무효화하고 리비전을 1 증가시킨다. 스케일 제외 행렬 캐시는 유지한다.
     */
    void SetScale(const FVector3& InScale);

    /**
     * @brief 부모와 결합할 때 위치를 절대 성분으로 사용할지 설정한다.
     * @param BInAbsoluteLocation true이면 위치에 부모의 스케일, 회전 및 이동을 적용하지 않는다.
     * @details 설정이 달라지면 리비전을 1 증가시킨다.
     */
    void SetAbsoluteLocation(bool BInAbsoluteLocation);

    /**
     * @brief 부모와 결합할 때 회전을 절대 성분으로 사용할지 설정한다.
     * @param BInAbsoluteRotation true이면 부모 회전을 결합하지 않는다.
     * @details 설정이 달라지면 리비전을 1 증가시킨다.
     */
    void SetAbsoluteRotation(bool BInAbsoluteRotation);

    /**
     * @brief 부모와 결합할 때 스케일을 절대 성분으로 사용할지 설정한다.
     * @param BInAbsoluteScale true이면 부모 스케일을 곱하지 않는다.
     * @details 설정이 달라지면 리비전을 1 증가시킨다.
     */
    void SetAbsoluteScale(bool BInAbsoluteScale);

    /**
     * @brief 스케일, 회전, 이동을 순서대로 적용하는 행렬을 조회한다.
     * @return 저장된 위치, 쿼터니언 및 스케일로 구성한 행렬.
     * @details 캐시가 무효하면 다시 계산하고, 유효하면 저장된 행렬을 재사용한다.
     */
    FMatrix ToMatrixWithScale() const;
    /**
     * @brief 회전과 이동만 적용하는 행렬을 조회한다.
     * @return 스케일을 모든 축에서 1로 사용하여 구성한 행렬.
     * @details 캐시가 무효하면 다시 계산하고, 유효하면 저장된 행렬을 재사용한다.
     */
    FMatrix ToMatrixNoScale() const;
    /**
     * @brief 스케일을 포함한 변환 행렬의 역행렬을 조회한다.
     * @return ToMatrixWithScale()의 역행렬 계산 결과.
     * @details 캐시가 무효하면 FMatrix::Invert()로 계산한다. 행렬식이 거의 0이면 현재 역행렬 구현은 단위 행렬을 반환한다.
     */
    FMatrix ToInverseMatrixWithScale() const;
    /**
     * @brief 현재 변환의 성분을 부모 변환과 결합한다.
     * @param Parent 결합할 부모 변환.
     * @return 결합한 위치, 회전, 스케일과 현재 객체의 절대 변환 설정을 가진 새 변환.
     * @details 위치는 부모 스케일과 회전을 적용한 뒤 부모 위치를 더하고, 회전은 부모 쿼터니언과 결합하며, 스케일은 축별로 곱한다. 절대 성분으로 설정된 항목은 현재 값을 유지한다.
     */
    FTransform Compose(const FTransform& Parent) const;
    /**
     * @brief 현재 변환을 지정한 부모에 대한 상대 변환으로 계산한다.
     * @param Parent 상대 변환의 기준이 되는 부모 변환.
     * @param[out] OutRelative 계산된 상대 변환. 실패 시 기존 값을 유지한다.
     * @return 상대 변환을 계산하면 true. 위치 또는 스케일이 비절대 성분이고 부모 스케일 중 절댓값이 1e-6 이하인 축이 있으면 false.
     * @details 절대 성분은 현재 값을 유지하고 나머지 성분에는 부모 변환의 역연산을 적용한다. 결과에 현재 객체의 절대 변환 설정을 반영한다.
     * @pre OutRelative는 현재 객체와 다른 객체여야 한다.
     */
    bool MakeRelativeTo(const FTransform& Parent, FTransform& OutRelative) const;
    /**
     * @brief 변환 성분과 절대 변환 설정을 직렬화한다.
     * @param[in,out] Archive 값을 읽거나 기록할 아카이브.
     * @details Position, Rotation, Scale, bAbsoluteLocation, bAbsoluteRotation, bAbsoluteScale 필드를 순서대로 처리한다. 로딩 시 읽은 값으로 새 변환을 구성하여 현재 객체에 대입한다.
     * @details 회전은 쿼터니언으로 직렬화하며 리비전과 행렬 캐시는 직렬화하지 않는다.
     */
    void Serialize(FArchive& Archive);

private:
    /**
     * @brief 두 객체의 변환 성분과 절대 변환 설정을 정확히 비교한다.
     * @param Other 비교할 변환.
     * @return 위치, 쿼터니언, 오일러 회전, 스케일 및 절대 변환 설정이 모두 같으면 true.
     * @details 리비전과 행렬 캐시는 비교하지 않는다.
     */
    bool HasSameState(const FTransform& Other) const;
    /**
     * @brief 스케일 포함 행렬과 그 역행렬 캐시를 무효화한다.
     * @param IncludeNoScale true이면 스케일 제외 행렬 캐시도 무효화한다.
     */
    void InvalidateMatrices(bool IncludeNoScale);

private:
    /** @brief 저장된 위치. 기본값은 원점이다. */
    FVector3 mPosition{0.0f, 0.0f, 0.0f};
    /** @brief 회전 계산에 사용하는 정규화된 쿼터니언. */
    FQuat mRotation{};
    /** @brief 쿼터니언과 함께 관리하는 도 단위 오일러 회전. */
    FRotator mRotationEuler{};
    /** @brief 각 축의 스케일. 기본값은 모든 축에서 1이다. */
    FVector3 mScale{1.0f, 1.0f, 1.0f};
    /** @brief 부모와 결합할 때 위치에 부모 변환을 적용하지 않는지 나타내는 설정. */
    bool mBAbsoluteLocation{false};
    /** @brief 부모와 결합할 때 회전을 상속하지 않는지 나타내는 설정. */
    bool mBAbsoluteRotation{false};
    /** @brief 부모와 결합할 때 스케일을 상속하지 않는지 나타내는 설정. */
    bool mBAbsoluteScale{false};
    /** @brief 현재 객체의 변환 상태 변경을 추적하는 리비전. */
    Uint64 mRevision{};
    /** @brief 스케일을 포함한 변환 행렬의 캐시. */
    mutable FMatrix mMatrixWithScale{};
    /** @brief 스케일을 제외한 변환 행렬의 캐시. */
    mutable FMatrix mMatrixNoScale{};
    /** @brief 스케일 포함 변환 행렬의 역행렬 캐시. */
    mutable FMatrix mInverseMatrixWithScale{};
    /** @brief 스케일 포함 행렬 캐시를 다시 계산해야 하는지 나타내는 상태. */
    mutable bool mMatrixWithScaleDirty{true};
    /** @brief 스케일 제외 행렬 캐시를 다시 계산해야 하는지 나타내는 상태. */
    mutable bool mMatrixNoScaleDirty{true};
    /** @brief 스케일 포함 역행렬 캐시를 다시 계산해야 하는지 나타내는 상태. */
    mutable bool mInverseMatrixWithScaleDirty{true};
};
