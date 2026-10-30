/**
 * @file FWorldSerializer.h
 * @brief 월드 액터와 컴포넌트의 직렬화 및 객체 참조 복원에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"

namespace Core {
    class FArchive;
}

namespace Engine {

    class FWorldContext;
    class AActor;

    /**
     * @brief 월드 액터와 컴포넌트의 직렬화 및 객체 참조 복원을 담당합니다.
     */
    class FWorldSerializer {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 선택적 부모 타입의 별칭을 선언합니다.
         */
        ReflectTypes(FWorldSerializer);

    public:
        /**
         * @brief 이 타입의 구조체 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FWorldSerializer);

        /**
         * @brief 월드의 액터·컴포넌트와 활성 카메라·입력 대상 참조를 직렬화합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in] Context 직렬화할 월드 컨텍스트입니다.
         */
        static void Serialize(Core::FArchive& Archive, const Engine::FWorldContext* Context);

        /**
         * @brief 액터 목록과 소유 컴포넌트 및 객체 식별자를 아카이브에 저장합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @param[in] Actors 직렬화하거나 수집할 액터 목록입니다.
         */
        static void SaveActors(Core::FArchive& Archive, const Core::TArray<Engine::AActor*>& Actors);

        /**
         * @brief 아카이브에서 액터와 컴포넌트를 생성하고 내부 객체 참조를 복원합니다.
         * @param[in,out] Archive 모드에 따라 값을 읽거나 쓸 아카이브입니다.
         * @return 로드한 액터들의 단독 소유 목록이며 오류는 아카이브에서 조회합니다.
         */
        static Core::TArray<std::unique_ptr<Engine::AActor>> LoadActors(Core::FArchive& Archive);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();
    };

}
