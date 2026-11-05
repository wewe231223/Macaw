/**
 * @file UScriptComponent.h
 * @brief C# 스크립트 인스턴스 하나를 액터에 연결하는 컴포넌트의 선언입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/Types.h"
#include "Engine/Component/UActorComponent.h"

#include <cstdint>

namespace Script {

    /**
     * @brief 이름으로 지정한 C# ScriptComponent 클래스를 실행 중에 인스턴스화하고 수명 호출을 전달합니다.
     * @note C# 인스턴스는 Game과 PIE의 BeginPlay부터 EndPlay까지만 존재합니다. 편집 월드에서는 만들지 않습니다.
     */
    class UScriptComponent final : public Engine::UActorComponent {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 기반 타입의 별칭입니다.
         */
        ReflectTypes(UScriptComponent, Engine::UActorComponent);

    public:
        /**
         * @brief 클래스가 지정되지 않은 컴포넌트를 구성합니다.
         */
        UScriptComponent();

        /**
         * @brief 남아 있는 C# 인스턴스를 해제합니다.
         */
        ~UScriptComponent() override;

    public:
        /**
         * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
         * @return 리플렉션 시스템이 보관하는 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(UScriptComponent);

        /**
         * @brief C# 인스턴스를 만들고 BeginPlay를 전달합니다.
         * @note 클래스가 비어 있거나 인스턴스를 만들지 못하면 스크립트 없이 계속 실행합니다.
         */
        void BeginPlay() override;

        /**
         * @brief C# 인스턴스에 Tick을 전달합니다.
         * @param[in] DeltaTime 이번 갱신이 진행하는 초 단위 시간입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief C# 인스턴스에 EndPlay를 전달하고 해제합니다.
         */
        void EndPlay() override;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 네임스페이스를 포함한 C# 클래스 이름입니다.
         */
        Core::FString mScriptClass{};
        ReflectProperty(mScriptClass, "ScriptClass", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 실행 중인 C# 인스턴스의 핸들이며, 없으면 0입니다.
         */
        std::intptr_t mInstance{};
    };

}
