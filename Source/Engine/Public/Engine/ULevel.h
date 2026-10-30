/**
 * @file ULevel.h
 * @brief 월드에 속한 액터의 소유권과 컴포넌트 등록 수명을 관리합니다.
 * @ingroup Engine
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "CoreUObject/UObject.h"

namespace Engine {

    class UWorld;
    class AActor;

    /**
     * @brief 월드에 속한 액터의 소유권과 컴포넌트 등록 수명을 관리합니다.
     * @ingroup Engine
     */
    class ULevel final : public CoreUObject::UObject {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(ULevel, CoreUObject::UObject);

    public:
        /**
         * @brief ULevel의 초기 상태와 전달받은 의존성을 구성합니다.
         */
        ULevel();

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~ULevel() override;

    public:
        /**
         * @brief 이 타입의 정적 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(ULevel, CoreUObject::EClassFlags::NoFactory);

        /**
         * @fn UClass* ULevel::GetClass() const
         * @brief 객체의 실제 클래스 메타데이터를 반환합니다.
         * @return 실제 클래스 메타데이터의 비소유 주소입니다.
         */


        /**
         * @brief 검증한 액터의 소유권을 레벨로 옮기고 컴포넌트를 월드에 등록합니다.
         * @param[in] Actor 추가할 액터의 소유권입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool AddActor(std::unique_ptr<Engine::AActor> Actor);

        /**
         * @brief 핸들에 해당하는 액터의 컴포넌트를 등록 해제하고 액터를 제거합니다.
         * @param[in] Handle 제거할 액터의 핸들입니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool DestroyActor(CoreUObject::FObjectHandle Handle);

        /**
         * @brief 레벨이 소유한 액터 목록을 반환합니다.
         * @return 소유 목록의 읽기 전용 참조입니다.
         */
        const Core::TArray<std::unique_ptr<Engine::AActor>>& GetActors() const;

    private:
        /**
         * @brief 생성된 프로퍼티 등록 함수와 빌더 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 객체가 소속된 월드 연결을 지정합니다.
         * @param[in] World 연결할 월드이며, nullptr이면 연결을 해제합니다.
         */
        void SetWorld(Engine::UWorld* World);

        /**
         * @brief 월드가 레벨 연결과 안전한 액터 제거를 관리하도록 허용합니다.
         */
        friend class UWorld;

        /**
         * @brief 안전한 삭제 경계에서 실행을 종료하고 액터의 소유권을 해제합니다.
         * @param[in] Handle 이 레벨에서 제거할 액터의 전역 핸들입니다.
         * @return 해당 액터를 찾아 제거하면 true입니다.
         */
        bool RemoveActor(CoreUObject::FObjectHandle Handle);

    private:
        /**
         * @brief 레벨이 속한 월드입니다.
         */
        Engine::UWorld* mWorld{};
        ReflectProperty(mWorld, "World", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 레벨이 소유한 액터 목록입니다.
         */
        Core::TArray<std::unique_ptr<Engine::AActor>> mActors{};
        ReflectProperty(mActors, "Actors", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
