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

    bool Engine::ALevelSequenceActor::InitializeDefaultComponents() {
        if (!GetComponents().empty()) {
            return false;
        }

        auto Root{CoreUObject::NewObject<Engine::USceneComponent>()};
        Root->SetName("Root");

        return AddComponent(std::move(Root));
    }

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

    void Engine::ALevelSequenceActor::Tick(float DeltaTime) {
        Engine::AActor::Tick(DeltaTime);
        mPlayer.Advance(DeltaTime);
    }

    void Engine::ALevelSequenceActor::EndPlay() {
        mPlayer.Reset();
        Engine::AActor::EndPlay();
    }

    Engine::FSequencePlayer& Engine::ALevelSequenceActor::GetPlayer() {
        return mPlayer;
    }

    const Core::FString& Engine::ALevelSequenceActor::GetSequencePath() const {
        return mSequencePath;
    }

    const Core::TArray<Engine::FSequenceBindingTarget>& Engine::ALevelSequenceActor::GetBindingTargets() const {
        return mBindingTargets;
    }

    void Engine::ALevelSequenceActor::SetBindingTarget(const Core::FGuid& BindingId, const Core::FGuid& ObjectId) {
        RemoveBindingTarget(BindingId);

        Engine::FSequenceBindingTarget Target{};
        Target.mBindingId = BindingId;
        Target.mObject = CoreUObject::FObjectReference{ObjectId};
        mBindingTargets.push_back(Target);
    }

    void Engine::ALevelSequenceActor::RemoveBindingTarget(const Core::FGuid& BindingId) {
        std::erase_if(mBindingTargets, [&BindingId](const Engine::FSequenceBindingTarget& Target) {
            return Target.mBindingId == BindingId;
        });
    }

}
