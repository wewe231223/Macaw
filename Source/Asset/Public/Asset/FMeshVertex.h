/**
 * @file FMeshVertex.h
 * @brief CPU 메시 정점의 위치와 표면 법선입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Math/FVector.h"

namespace Asset {

    /**
     * @brief CPU 메시 정점의 위치와 표면 법선입니다.
     * @ingroup Asset
     */
    struct FMeshVertex {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FMeshVertex);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FMeshVertex);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 정점 위치입니다.
         */
        Core::FVector mPosition{};
        ReflectProperty(mPosition, "Position", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 법선입니다.
         */
        Core::FVector mNormal{};
        ReflectProperty(mNormal, "Normal", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
