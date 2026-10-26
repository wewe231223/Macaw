/**
 * @file FSceneView.h
 * @brief 카메라 위치와 방향, 뷰·투영 행렬 및 뷰포트 크기입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"

struct FRay;

/**
 * @brief 카메라 위치와 방향, 뷰·투영 행렬 및 뷰포트 크기입니다.
 * @ingroup Engine
 */
struct FSceneView {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(FSceneView);

public:
    /**
     * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
     * @return 등록된 타입 메타데이터의 비소유 주소입니다.
     */
    ReflectStruct(FSceneView);
    bool Deproject(const FVector2& Position, FRay& Ray) const;

private:
    /**
     * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
     */
    ReflectMembers();

public:
    /**
     * @brief 공간상의 위치입니다.
     */
    FVector mLocation{};
    ReflectProperty(mLocation, "Location", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 카메라의 전방 방향입니다.
     */
    FVector mForward{1.0f, 0.0f, 0.0f};
    ReflectProperty(mForward, "Forward", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 카메라 뷰 또는 뷰 행렬입니다.
     */
    FMatrix mView{};
    ReflectProperty(mView, "View", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 원근 투영 행렬입니다.
     */
    FMatrix mProjection{};
    ReflectProperty(mProjection, "Projection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 뷰와 투영을 합성한 행렬입니다.
     */
    FMatrix mViewProjection{};
    ReflectProperty(mViewProjection, "ViewProjection", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);


    /**
     * @brief 뷰포트의 가로·세로 픽셀 크기입니다.
     */
    FVector2 mSize{};
    ReflectProperty(mSize, "Size", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);

    /**
     * @brief 수직 시야각입니다.
     */
    float mFieldOfView{60.0f};
    ReflectProperty(mFieldOfView, "FieldOfView", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mOrthographic{};
    ReflectProperty(mOrthographic, "Orthographic", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    float mOrthographicWidth{20.0f};
    ReflectProperty(mOrthographicWidth, "OrthographicWidth", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
