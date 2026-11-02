/**
 * @file FMeshPassParameters.h
 * @brief 메시 패스를 한 번 그릴 때마다 달라지는 출력 타깃과 뷰 투영을 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "FPassTarget.h"

namespace Renderer {

    /**
     * @brief 메시 패스를 한 번 그릴 때마다 달라지는 출력 타깃과 뷰 투영 묶음입니다.
     */
    struct FMeshPassParameters {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FMeshPassParameters);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshPassParameters);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 패스 결과를 기록할 출력 타깃입니다.
         */
        Renderer::FPassTarget mTarget{};
        ReflectProperty(mTarget, "Target", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 변환과 프러스텀 컬링에 함께 사용하는 월드→클립 행렬입니다.
         */
        Core::FMatrix mViewProjection{};
        ReflectProperty(mViewProjection, "ViewProjection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
