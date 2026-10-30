/**
 * @file UCameraComponent.h
 * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/USceneComponent.h"

struct FSceneView;
struct FVector2;

/**
 * @brief 시야각 설정과 월드 변환으로부터 카메라 뷰 생성을 담당합니다.
 */
class UCameraComponent final : public USceneComponent {
public:
    /**
     * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
     */
    ReflectTypes(UCameraComponent, USceneComponent);

public:
    /**
     * @brief 이 타입의 클래스 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectClass(UCameraComponent);

    /**
     * @brief 카메라의 수직 시야각을 도 단위로 반환합니다.
     * @return 카메라의 시야각이며 도 단위입니다.
     */
    float GetFieldOfView() const;

    /**
     * @brief 유효한 범위의 시야각을 검사하여 카메라에 설정합니다.
     * @param[in] FieldOfView 설정할 카메라 시야각이며 도 단위입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool SetFieldOfView(float FieldOfView);

    /**
     * @brief 카메라의 월드 변환과 시야각으로 뷰·투영 행렬을 계산합니다.
     * @param[in] Size 계산할 뷰포트의 가로·세로 픽셀 크기입니다.
     * @param[out] View 계산한 카메라 뷰·투영 상태를 받을 출력입니다.
     * @return 요청한 처리를 완료하면 true, 실패하면 false입니다.
     */
    bool GetCameraView(const FVector2& Size, FSceneView& View) const;

private:
    /**
     * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
     */
    ReflectMembers();

private:
    /**
     * @brief 카메라의 수직 시야각이며 도 단위입니다.
     */
    float mFieldOfView{60.0f};
    ReflectAccessor(mFieldOfView, GetFieldOfView, SetFieldOfView, "FieldOfView", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
