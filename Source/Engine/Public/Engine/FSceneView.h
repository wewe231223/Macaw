/**
 * @file FSceneView.h
 * @brief 카메라 위치와 방향, 뷰·투영 행렬 및 뷰포트 크기입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector2.h"
#include "Core/Math/FMatrix.h"

namespace Core {
    struct FRay;
}

namespace Engine {

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

        /**
         * @brief 뷰포트 픽셀 좌표를 월드 공간의 정규화된 광선으로 역투영합니다.
         * @param[in] Position 뷰포트 픽셀 좌표입니다.
         * @param[out] Ray 계산한 월드 광선입니다.
         * @return 검증 또는 요청한 처리에 성공하면 true입니다.
         */
        bool Deproject(const Core::FVector2& Position, Core::FRay& Ray) const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 공간상의 위치입니다.
         */
        Core::FVector mLocation{};
        ReflectProperty(mLocation, "Location", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 카메라의 전방 방향입니다.
         */
        Core::FVector mForward{1.0f, 0.0f, 0.0f};
        ReflectProperty(mForward, "Forward", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 카메라 뷰 또는 뷰 행렬입니다.
         */
        Core::FMatrix mView{};
        ReflectProperty(mView, "View", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 원근 투영 행렬입니다.
         */
        Core::FMatrix mProjection{};
        ReflectProperty(mProjection, "Projection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 뷰와 투영을 합성한 행렬입니다.
         */
        Core::FMatrix mViewProjection{};
        ReflectProperty(mViewProjection, "ViewProjection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 뷰포트의 가로·세로 픽셀 크기입니다.
         */
        Core::FVector2 mSize{};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 수직 시야각입니다.
         */
        float mFieldOfView{60.0f};
        ReflectProperty(mFieldOfView, "FieldOfView", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정사영 카메라 여부입니다.
         */
        bool mOrthographic{};
        ReflectProperty(mOrthographic, "Orthographic", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정사영 가로 가시 범위의 월드 길이입니다.
         */
        float mOrthographicWidth{20.0f};
        ReflectProperty(mOrthographicWidth, "OrthographicWidth", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
