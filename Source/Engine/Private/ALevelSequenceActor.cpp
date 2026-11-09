/**
 * @file ALevelSequenceActor.cpp
 * @brief 씬에 배치되어 시퀀스 에셋을 재생하는 액터에 대한 선언입니다.
 * @ingroup Engine
 */

#include "Engine/ALevelSequenceActor.h"
#include "Asset/FAssetManager.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Core/Archive.h"
#include "Core/Base/ErrorHandler.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/UWorld.h"

#include <string>
#include <utility>

namespace Engine {

    /**
     * @brief 액터의 기본 컴포넌트를 생성하고 루트 컴포넌트를 정합니다.
     */
    bool Engine::ALevelSequenceActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Root{CoreUObject::NewObject<Engine::USceneComponent>()};
        Root->SetName("Root");

        return AddComponent(std::move(Root));
    }

    /**
     * @brief 액터의 게임 실행 시작 시 호출되는 확장 지점입니다.
     */
    void Engine::ALevelSequenceActor::BeginPlay() {
        Engine::AActor::BeginPlay();

        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (Assets == nullptr || mSequencePath.empty()) {
            return;
        }

        Core::FString Error{};
        const Asset::ULevelSequence* Sequence{Assets->LoadSequence(Core::FilePath::FromUtf8(mSequencePath), Error)};

        if (Sequence == nullptr) {
            Core::ErrorHandler::Report("Sequencer", "Cannot load sequence '" + mSequencePath + "': " + Error, Core::ErrorHandler::EErrorLevel::Warning);

            return;
        }

        mPlayer.SetLooping(mLoop);
        mPlayer.SetRestoreOnFinish(mRestoreOnFinish);

        if (mPlayer.Initialize(Sequence, GetWorld(), mBindingTargets) && mAutoPlay) {
            mPlayer.Play();
        }
    }

    /**
     * @brief 액터의 실행 또는 허용된 편집 갱신 시 호출되는 확장 지점입니다.
     */
    void Engine::ALevelSequenceActor::Tick(float DeltaTime) {
        Engine::AActor::Tick(DeltaTime);
        mPlayer.Advance(DeltaTime);
    }

    /**
     * @brief 액터의 게임 실행 종료 시 호출되는 확장 지점입니다.
     */
    void Engine::ALevelSequenceActor::EndPlay() {
        mPlayer.Reset();
        Engine::AActor::EndPlay();
    }

    /**
     * @brief 시퀀스 플레이어를 반환합니다.
     */
    Engine::FSequencePlayer& Engine::ALevelSequenceActor::GetPlayer() {
        return mPlayer;
    }

    /**
     * @brief 재생할 시퀀스 에셋의 경로를 반환합니다.
     */
    const Core::FString& Engine::ALevelSequenceActor::GetSequencePath() const {
        return mSequencePath;
    }

    /**
     * @brief 시퀀스 바인딩과 씬 객체의 연결 목록을 반환합니다.
     */
    const Core::TArray<Engine::FSequenceBindingTarget>& Engine::ALevelSequenceActor::GetBindingTargets() const {
        return mBindingTargets;
    }

    /**
     * @brief 바인딩에 씬 객체를 연결하고 기존 연결은 교체합니다.
     */
    void Engine::ALevelSequenceActor::SetBindingTarget(const Core::FGuid& BindingId, const Core::FGuid& ObjectId) {
        RemoveBindingTarget(BindingId);

        Engine::FSequenceBindingTarget Target{};
        Target.mBindingId = BindingId;
        Target.mObject = CoreUObject::FObjectReference{ObjectId};

        mBindingTargets.push_back(Target);
    }

    /**
     * @brief 바인딩의 연결을 제거합니다.
     */
    void Engine::ALevelSequenceActor::RemoveBindingTarget(const Core::FGuid& BindingId) {
        std::erase_if(mBindingTargets, [&BindingId](const Engine::FSequenceBindingTarget& Target) {
            return Target.mBindingId == BindingId;
        });
    }

}
