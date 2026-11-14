/**
 * @file ALevelSequenceActor.h
 * @brief 씬에 배치되어 시퀀스 에셋을 재생하는 액터에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "CoreUObject/TObjectRef.h"

#include "Engine/AActor.h"
#include "Engine/Sequence/FSequencePlayer.h"
#include "ALevelSequenceActor.generated.h"

namespace Engine {

    /**
     * @brief 씬에 배치되어 시퀀스 에셋을 재생하고 바인딩 대상을 연결합니다.
     * @ingroup Engine
     */
    UCLASS()
    class ALevelSequenceActor final : public Engine::AActor {
        GENERATED_BODY()

    public:
        /**
         * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
         * @return 조건을 만족하거나 작업이 성공하면 true, 그렇지 않으면 false입니다.
         */
        bool InitializeDefaultComponents() override;

        /**
         * @brief 액터의 게임 실행 시작 시 호출되는 확장 지점입니다.
         */
        void BeginPlay() override;

        /**
         * @brief 액터의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
         * @param[in] DeltaTime 이번 갱신이 진행하는 초 단위 시간입니다.
         */
        void Tick(float DeltaTime) override;

        /**
         * @brief 액터의 게임 실행 종료 시 호출되는 확장 지점입니다.
         */
        void EndPlay() override;

        /**
         * @brief 시퀀스 플레이어를 반환합니다.
         * @return 액터가 소유한 플레이어의 참조입니다.
         */
        Engine::FSequencePlayer& GetPlayer();

        /**
         * @brief 재생할 시퀀스 에셋의 경로를 반환합니다.
         * @return Content 상대 경로의 참조입니다.
         */
        const Core::FString& GetSequencePath() const;

        /**
         * @brief 시퀀스 바인딩과 씬 객체의 연결 목록을 반환합니다.
         * @return 액터가 소유한 연결 목록의 참조입니다.
         */
        const Core::TArray<Engine::FSequenceBindingTarget>& GetBindingTargets() const;

        /**
         * @brief 바인딩에 씬 객체를 연결하고 기존 연결은 교체합니다.
         * @param[in] BindingId 시퀀스 바인딩 ID입니다.
         * @param[in] ObjectId 연결할 액터 또는 공간 컴포넌트의 GUID입니다.
         */
        void SetBindingTarget(const Core::FGuid& BindingId, CoreUObject::TObjectRef<CoreUObject::UObject> Object);

        /**
         * @brief 바인딩의 연결을 제거합니다.
         * @param[in] BindingId 시퀀스 바인딩 ID입니다.
         */
        void RemoveBindingTarget(const Core::FGuid& BindingId);

    private:
        /**
         * @brief 재생할 시퀀스 에셋의 Content 상대 경로입니다.
         */
        UPROPERTY(Edit, Serialize, Name = "Sequence")
        Core::FString mSequencePath{};

        /**
         * @brief 게임 시작 시 자동으로 재생할지 나타냅니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mAutoPlay{true};

        /**
         * @brief 끝에 도달했을 때 처음부터 반복할지 나타냅니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mLoop{};

        /**
         * @brief 재생이 끝났을 때 대상의 변환을 원래대로 복원할지 나타냅니다.
         */
        UPROPERTY(Edit, Serialize)
        bool mRestoreOnFinish{};

        /**
         * @brief 시퀀스 바인딩과 씬 객체의 연결 목록입니다.
         */
        UPROPERTY(Serialize)
        Core::TArray<Engine::FSequenceBindingTarget> mBindingTargets{};

        /**
         * @brief 실행 중 시퀀스를 재생하는 플레이어입니다.
         */
        Engine::FSequencePlayer mPlayer{};
    };

}
