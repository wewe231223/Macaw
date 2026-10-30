/**
 * @file FObjectConstants.h
 * @brief 객체별 월드 행렬, 역행렬과 UV 변환을 전달하는 GPU 상수 버퍼 레이아웃입니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector4.h"

#include <cstddef>
#include <type_traits>

namespace Renderer {

    /**
     * @brief 객체별 월드 행렬, 역행렬과 UV 변환을 전달하는 GPU 상수 버퍼 레이아웃입니다.
     * @ingroup Renderer
     */
    struct FObjectConstants {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FObjectConstants);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FObjectConstants);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 오브젝트의 월드 변환 행렬입니다.
         */
        Core::FMatrix mWorld{};
        ReflectProperty(mWorld, "World", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 월드 변환의 역행렬입니다.
         */
        Core::FMatrix mWorldInverse{};
        ReflectProperty(mWorldInverse, "WorldInverse", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief XY 배율과 ZW 오프셋으로 구성된 UV 변환입니다.
         */
        Core::FVector4 mUVTransform{1.0f, 1.0f, 0.0f, 0.0f};
        ReflectProperty(mUVTransform, "UVTransform", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자 수신 여부와 셰이더 정렬을 위한 예약값을 저장합니다.
         */
        Core::FVector4 mShadowFlags{1.0f, 0.0f, 0.0f, 0.0f};
        ReflectProperty(mShadowFlags, "ShadowFlags", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

    };

    static_assert(sizeof(Core::FMatrix) == 64 && alignof(Core::FMatrix) == 16 && std::is_trivially_copyable_v<Core::FMatrix>);
    static_assert(std::is_standard_layout_v<Renderer::FObjectConstants> && sizeof(Renderer::FObjectConstants) == 160);
    static_assert(offsetof(Renderer::FObjectConstants, mWorld) == 0 && offsetof(Renderer::FObjectConstants, mWorldInverse) == 64);

}
