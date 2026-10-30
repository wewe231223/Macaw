/**
 * @file FEditorCameraState.h
 * @brief 장면에 보관하는 원근 편집 카메라 상태입니다.
 * @ingroup Engine
 */
#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"

class FArchive;

/**
 * @brief 위치·회전과 투영 설정을 검증하고 직렬화합니다.
 */
class FEditorCameraState {
public:
    /**
     * @brief 리플렉션 소유 타입의 별칭입니다.
     */
    ReflectTypes(FEditorCameraState);

public:
    /**
     * @brief 기본 원근 편집 카메라를 구성합니다.
     */
    FEditorCameraState() = default;

public:
    /**
     * @brief 정적 타입 메타데이터를 반환합니다.
     * @return 등록된 구조체 메타데이터입니다.
     */
    ReflectStruct(FEditorCameraState);
    /**
     * @brief 위치를 반환합니다.
     * @return 월드 공간 위치의 참조입니다.
     */
    const FVector& GetLocation() const;
    /**
     * @brief 회전을 반환합니다.
     * @return 도 단위 회전의 참조입니다.
     */
    const FRotator& GetRotation() const;
    /**
     * @brief 시야각을 반환합니다.
     * @return 도 단위 수직 시야각입니다.
     */
    float GetFieldOfView() const;
    /**
     * @brief 근평면 거리를 반환합니다.
     * @return 양수인 근평면 거리입니다.
     */
    float GetNearClip() const;
    /**
     * @brief 원평면 거리를 반환합니다.
     * @return 근평면보다 먼 원평면 거리입니다.
     */
    float GetFarClip() const;
    /**
     * @brief 카메라의 유한성과 투영 범위를 검사합니다.
     * @return 유효한 상태이면 true입니다.
     */
    bool IsValid() const;
    /**
     * @brief 저장 대상 카메라 필드가 같은지 비교합니다.
     * @param Other 비교할 카메라입니다.
     * @return 모든 저장 필드가 같으면 true입니다.
     */
    bool operator==(const FEditorCameraState& Other) const;
    /**
     * @brief 검증한 카메라 상태를 원자적으로 적용합니다.
     * @param Location 월드 위치입니다.
     * @param Rotation 도 단위 회전입니다.
     * @param FieldOfView 수직 시야각입니다.
     * @param NearClip 근평면 거리입니다.
     * @param FarClip 원평면 거리입니다.
     * @return 유효한 상태를 적용하면 true입니다.
     */
    bool Set(const FVector& Location, const FRotator& Rotation, float FieldOfView, float NearClip, float FarClip);
    /**
     * @brief 카메라 필드를 저장하거나 복원합니다.
     * @param Archive 저장 또는 복원할 아카이브입니다.
     */
    void Serialize(FArchive& Archive);

private:
    /**
     * @brief 생성된 리플렉션 멤버 등록 함수의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 월드 위치입니다.
     */
    FVector mLocation{-7.0f, -9.0f, 6.0f};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 도 단위 회전입니다.
     */
    FRotator mRotation{-26.0f, 50.0f, 0.0f};
    ReflectProperty(mRotation, "Rotation", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 도 단위 수직 시야각입니다.
     */
    float mFieldOfView{60.0f};
    ReflectProperty(mFieldOfView, "FieldOfView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 원근 근평면 거리입니다.
     */
    float mNearClip{0.05f};
    ReflectProperty(mNearClip, "NearClip", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 원근 원평면 거리입니다.
     */
    float mFarClip{10000.0f};
    ReflectProperty(mFarClip, "FarClip", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
