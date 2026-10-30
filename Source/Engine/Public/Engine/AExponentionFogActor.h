/**
 * @file AExponentionFogActor.h
 * @brief 높이 기반 지수 안개 컴포넌트를 루트로 생성하여 에디터에서 배치하는 액터입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"

namespace Engine {

    class UHeightFogComponent;

    /**
     * @brief 높이 기반 지수 안개 컴포넌트를 루트로 생성하여 에디터에서 배치하는 액터입니다.
     */
    class AExponentionFogActor final : public Engine::AActor {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(AExponentionFogActor, Engine::AActor);

    public:
        /**
         * @brief 기본 설정으로 AExponentionFogActor 객체를 초기화합니다.
         */
        AExponentionFogActor();
        /**
         * @brief 객체 수명을 끝내고 소유한 자원을 정리합니다.
         */
        ~AExponentionFogActor() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(AExponentionFogActor);

        /**
         * @brief 빈 액터에 HeightFogComponent를 생성하고 루트 컴포넌트로 추가합니다.
         * @return 컴포넌트를 추가하면 true이며 이미 컴포넌트가 있거나 추가에 실패하면 false입니다.
         */
        bool InitializeDefaultComponents() override;
        /**
         * @brief 안개 액터의 루트에 연결된 높이 안개 컴포넌트를 조회합니다.
         * @return 루트가 높이 안개 컴포넌트이면 비소유 주소를 반환하며 그렇지 않으면 nullptr입니다.
         */
        Engine::UHeightFogComponent* GetFogComponent() const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

}
