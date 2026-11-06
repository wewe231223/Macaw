/**
 * @file UScriptComponent.h
 * @brief C# 스크립트 인스턴스 하나를 액터에 연결하는 컴포넌트의 선언입니다.
 * @ingroup Script
 */

#pragma once

#include "Core/Types.h"
#include "Engine/Component/UActorComponent.h"
#include "Script/FScriptField.h"

#include <cstdint>
#include "UScriptComponent.generated.h"

namespace Script {

    /**
     * @brief 이름으로 지정한 C# ScriptComponent 클래스를 실행 중에 인스턴스화하고 수명 호출을 전달합니다.
     * @note C# 인스턴스는 Game과 PIE의 BeginPlay부터 EndPlay까지만 존재합니다. 편집 월드에서는 만들지 않습니다.
     */
    UCLASS()
    class UScriptComponent final : public Engine::UActorComponent {
        GENERATED_BODY()

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

        /**
         * @brief 스크립트 클래스가 바뀌면 이전 클래스의 필드 값을 버립니다.
         * @param[in] Event 변경된 프로퍼티 정보입니다.
         */
        void PostEditChangeProperty(const CoreUObject::FPropertyChangedEvent& Event) override;

        /**
         * @brief 네임스페이스를 포함한 C# 클래스 이름을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::FString& GetScriptClass() const;

        /**
         * @brief 기본값과 다르게 저장된 필드 목록을 반환합니다.
         * @return 소유 객체가 유지하는 값의 참조입니다.
         */
        const Core::TArray<Script::FScriptField>& GetFields() const;

        /**
         * @brief 필드 값을 저장합니다. 같은 이름이 있으면 교체합니다.
         * @param[in] Field 저장할 필드입니다.
         */
        void SetField(const Script::FScriptField& Field);

        /**
         * @brief 저장된 필드를 지워 스크립트의 기본값으로 되돌립니다.
         * @param[in] Name C# 필드 이름입니다.
         */
        void RemoveField(const Core::FString& Name);

    private:
        /**
         * @brief 소유 액터의 충돌 도형 이벤트를 C# 인스턴스로 연결합니다.
         */
        void BindOverlapEvents();

        /**
         * @brief 충돌 도형에 등록한 이벤트를 모두 해제합니다.
         */
        void UnbindOverlapEvents();

    private:
        /**
         * @brief 네임스페이스를 포함한 C# 클래스 이름입니다.
         */
        UPROPERTY(Edit, Serialize)
        Core::FString mScriptClass{};

        /**
         * @brief 기본값과 다르게 편집한 필드 값입니다.
         */
        UPROPERTY(Serialize)
        Core::TArray<Script::FScriptField> mFields{};

        /**
         * @brief 실행 중인 C# 인스턴스의 핸들이며, 없으면 0입니다.
         */
        std::intptr_t mInstance{};

        /**
         * @brief 충돌 도형에 등록한 겹침 이벤트입니다.
         */
        struct FOverlapBinding {
            /**
             * @brief 충돌 도형의 핸들입니다.
             */
            CoreUObject::FObjectHandle mShape{};

            /**
             * @brief 겹침 시작 이벤트의 등록 번호입니다.
             */
            Core::Uint32 mBegin{};

            /**
             * @brief 겹침 끝 이벤트의 등록 번호입니다.
             */
            Core::Uint32 mEnd{};
        };

        /**
         * @brief 해제할 겹침 이벤트 목록입니다.
         */
        Core::TArray<FOverlapBinding> mOverlapBindings{};
    };

}
