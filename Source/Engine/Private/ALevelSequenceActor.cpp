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

    void Engine::ALevelSequenceActor::Serialize(Core::FArchive& Archive) {
        Engine::AActor::Serialize(Archive);

        if (Archive.HasError() || (Archive.IsLoading() && !Archive.HasField("BindingTargets"))) {
            return;
        }

        Core::Uint32 Count{static_cast<Core::Uint32>(mBindingTargets.size())};

        if (!Archive.BeginArray("BindingTargets", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mBindingTargets.assign(Count, Engine::FSequenceBindingTarget{});
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Engine::FSequenceBindingTarget& Target{mBindingTargets[Index]};
            Archive.Serialize("Binding", Target.mBindingId);
            Archive.Serialize("Object", Target.mObjectId);

            // 대상이 삭제되었거나 함께 복제되지 않아도 씬 복제가 실패하지 않도록 약한 참조로 변환합니다.
            if (Archive.IsLoading() && !Archive.HasError()) {
                Archive.RemapWeakObjectReference(Target.mObjectId);
            }

            Archive.EndObject();
        }

        Archive.EndArray();
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
        Target.mObjectId = ObjectId;
        mBindingTargets.push_back(Target);
    }

    void Engine::ALevelSequenceActor::RemoveBindingTarget(const Core::FGuid& BindingId) {
        std::erase_if(mBindingTargets, [&BindingId](const Engine::FSequenceBindingTarget& Target) {
            return Target.mBindingId == BindingId;
        });
    }

}
