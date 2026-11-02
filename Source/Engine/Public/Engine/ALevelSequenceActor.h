/**
 * @file ALevelSequenceActor.h
 * @brief 씬에 배치되어 시퀀스 에셋을 재생하는 액터에 대한 선언입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/AActor.h"
#include "Engine/Sequence/FSequencePlayer.h"

namespace Engine {

    /**
     * @brief 씬에 배치되어 시퀀스 에셋을 재생하고 바인딩 대상을 연결합니다.
     * @ingroup Engine
     */
    class ALevelSequenceActor final : public Engine::AActor {
    public:
        /**
         * @brief 리플렉션에서 사용하는 소유 타입과 부모 타입의 별칭입니다.
         */
        ReflectTypes(ALevelSequenceActor, Engine::AActor);

    public:
        /**
         * @brief 이 타입의 클래스 메타데이터를 반환합니다.
         * @return 등록된 타입 메타데이터의 비소유 주소입니다.
         */
        ReflectClass(ALevelSequenceActor);

        bool InitializeDefaultComponents() override;

        void BeginPlay() override;

        void Tick(float DeltaTime) override;

        void EndPlay() override;

        void Serialize(Core::FArchive& Archive) override;

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
        void SetBindingTarget(const Core::FGuid& BindingId, const Core::FGuid& ObjectId);

        /**
         * @brief 바인딩의 연결을 제거합니다.
         * @param[in] BindingId 시퀀스 바인딩 ID입니다.
         */
        void RemoveBindingTarget(const Core::FGuid& BindingId);

    private:
        /**
         * @brief 자동 생성된 프로퍼티 등록 코드와 리플렉션 빌더의 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 재생할 시퀀스 에셋의 Content 상대 경로입니다.
         */
        Core::FString mSequencePath{};
        ReflectProperty(mSequencePath, "Sequence", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 게임 시작 시 자동으로 재생할지 나타냅니다.
         */
        bool mAutoPlay{true};
        ReflectProperty(mAutoPlay, "AutoPlay", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 끝에 도달했을 때 처음부터 반복할지 나타냅니다.
         */
        bool mLoop{};
        ReflectProperty(mLoop, "Loop", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 재생이 끝났을 때 대상의 변환을 원래대로 복원할지 나타냅니다.
         */
        bool mRestoreOnFinish{};
        ReflectProperty(mRestoreOnFinish, "RestoreOnFinish", Core::EPropertyFlags::Edit | Core::EPropertyFlags::Serialize);

        /**
         * @brief 시퀀스 바인딩과 씬 객체의 연결 목록입니다.
         */
        Core::TArray<Engine::FSequenceBindingTarget> mBindingTargets{};
        ReflectProperty(mBindingTargets, "BindingTargets", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 실행 중 시퀀스를 재생하는 플레이어입니다.
         */
        Engine::FSequencePlayer mPlayer{};
        ReflectProperty(mPlayer, "Player", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
