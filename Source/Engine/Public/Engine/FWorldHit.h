/**
 * @file FWorldHit.h
 * @brief 월드 광선 질의로 찾은 액터, 컴포넌트와 충돌 위치 및 거리입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/FObjectHandle.h"
#include "Core/Math/FVector.h"

namespace Engine {

    /**
     * @brief 월드 광선 질의로 찾은 액터, 컴포넌트와 충돌 위치 및 거리입니다.
     * @ingroup Engine
     */
    struct FWorldHit {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FWorldHit);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FWorldHit);

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 충돌한 액터 핸들입니다.
         */
        CoreUObject::FObjectHandle mActor{};
        ReflectProperty(mActor, "Actor", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 충돌한 컴포넌트 핸들입니다.
         */
        CoreUObject::FObjectHandle mComponent{};
        ReflectProperty(mComponent, "Component", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);


        /**
         * @brief 월드 좌표의 충돌 위치입니다.
         */
        Core::FVector mPosition{};
        ReflectProperty(mPosition, "Position", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 광선상의 충돌 거리입니다.
         */
        float mDistance{};
        ReflectProperty(mDistance, "Distance", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
