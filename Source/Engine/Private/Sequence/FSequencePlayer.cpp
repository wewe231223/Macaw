#include "Engine/Sequence/FSequencePlayer.h"
#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Core/Base/ErrorHandler.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FProperty.h"
#include "CoreUObject/Reflection/UClass.h"
#include "Engine/AActor.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    namespace {
        Engine::USceneComponent* FindSceneComponent(const Engine::UWorld* World, const Core::FGuid& Guid) {
            const Engine::ULevel* Level{World->GetPersistentLevel()};

            if (Level == nullptr || !Guid.IsValid()) {
                return nullptr;
            }

            for (const auto& Actor : Level->GetActors()) {
                if (Actor->GetGuid() == Guid) {
                    return Actor->GetRootComponent();
                }

                for (const auto& Component : Actor->GetComponents()) {
                    if (Component->GetGuid() == Guid) {
                        return CoreUObject::Cast<Engine::USceneComponent>(Component.get());
                    }
                }
            }

            return nullptr;
        }
    }

    bool Engine::FSequencePlayer::Initialize(const Asset::ULevelSequence* Sequence, Engine::UWorld* World, const Core::TArray<Engine::FSequenceBindingTarget>& Targets) {
        Reset();

        if (Sequence == nullptr || World == nullptr) {
            return false;
        }

        mSequence = Sequence;
        mWorld = World;
        mTick = Sequence->GetStartTick();

        for (const Asset::FSequenceBinding& Binding : Sequence->GetBindings()) {
            const auto Target{std::find_if(Targets.begin(), Targets.end(), [&Binding](const Engine::FSequenceBindingTarget& Candidate) {
                return Candidate.mBindingId == Binding.GetId();
            })};
            const Engine::USceneComponent* Component{Target != Targets.end() ? FindSceneComponent(World, Target->mObjectId) : nullptr};

            if (Component == nullptr) {
                Core::ErrorHandler::Report("Sequencer", "Cannot resolve sequence binding '" + Binding.GetName() + "'", Core::ErrorHandler::EErrorLevel::Warning);
                continue;
            }

            FResolvedBinding Resolved{};
            Resolved.mBinding = &Binding;
            Resolved.mComponent = Component->GetHandle();
            mBindings.push_back(Resolved);
        }

        return true;
    }

    void Engine::FSequencePlayer::Reset() {
        mSequence = nullptr;
        mWorld = nullptr;
        mBindings.clear();
        mBaseCamera = {};
        mCutCamera = {};
        mCameraCaptured = false;
        mTick = 0.0;
        mState = Engine::ESequencePlayState::Stopped;
    }

    bool Engine::FSequencePlayer::Play() {
        if (mSequence == nullptr) {
            return false;
        }

        if (mState == Engine::ESequencePlayState::Stopped) {
            mTick = mSequence->GetStartTick();
        }

        mState = Engine::ESequencePlayState::Playing;
        Evaluate();

        return true;
    }

    void Engine::FSequencePlayer::Pause() {
        if (mState == Engine::ESequencePlayState::Playing) {
            mState = Engine::ESequencePlayState::Paused;
        }
    }

    void Engine::FSequencePlayer::Stop() {
        mState = Engine::ESequencePlayState::Stopped;

        if (mRestoreOnFinish) {
            RestoreState();
        }
    }

    void Engine::FSequencePlayer::Advance(float DeltaTime) {
        if (mSequence == nullptr || mState != Engine::ESequencePlayState::Playing || !std::isfinite(DeltaTime) || DeltaTime < 0.0f) {
            return;
        }

        const double Start{static_cast<double>(mSequence->GetStartTick())};
        const double End{static_cast<double>(mSequence->GetEndTick())};
        mTick += static_cast<double>(DeltaTime) * Asset::ULevelSequence::TicksPerSecond;

        if (mTick < End) {
            Evaluate();
            return;
        }

        if (mLooping && End > Start) {
            mTick = Start + std::fmod(mTick - Start, End - Start);
            Evaluate();
            return;
        }

        // 마지막 키의 값이 항상 적용되도록 끝 시간을 평가한 뒤 재생을 마칩니다.
        mTick = End;
        Evaluate();
        Stop();
    }

    void Engine::FSequencePlayer::Seek(double Tick) {
        if (mSequence == nullptr) {
            return;
        }

        mTick = std::clamp(Tick, static_cast<double>(mSequence->GetStartTick()), static_cast<double>(mSequence->GetEndTick()));

        if (mState == Engine::ESequencePlayState::Stopped) {
            mState = Engine::ESequencePlayState::Paused;
        }

        Evaluate();
    }

    Engine::USceneComponent* Engine::FSequencePlayer::FindBoundComponent(const Core::FGuid& BindingId) const {
        for (const FResolvedBinding& Binding : mBindings) {
            if (Binding.mBinding->GetId() == BindingId) {
                return CoreUObject::Cast<Engine::USceneComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Binding.mComponent));
            }
        }

        return nullptr;
    }

    void Engine::FSequencePlayer::RestoreState() {
        for (FResolvedBinding& Binding : mBindings) {
            auto* Component{CoreUObject::Cast<Engine::USceneComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Binding.mComponent))};

            if (Binding.mCaptured && Component != nullptr) {
                Component->SetTransform(Binding.mBaseTransform);
            }

            for (const FSavedProperty& Saved : Binding.mBaseProperties) {
                const CoreUObject::FProperty* Property{Component != nullptr ? Component->GetClass()->FindClassProperty(Saved.mName) : nullptr};

                if (Property != nullptr) {
                    Property->WriteValue(Component, CoreUObject::FPropertyValue{Saved.mValue});
                }
            }

            Binding.mCaptured = false;
            Binding.mBaseProperties.clear();
        }

        if (mCameraCaptured && mWorld != nullptr) {
            mWorld->SetActiveCamera(CoreUObject::Cast<Engine::UCameraComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mBaseCamera)));
        }

        mCameraCaptured = false;
        mCutCamera = {};
    }

    Engine::UCameraComponent* Engine::FSequencePlayer::GetCutCamera() const {
        return CoreUObject::Cast<Engine::UCameraComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mCutCamera));
    }

    void Engine::FSequencePlayer::SetLooping(bool Looping) {
        mLooping = Looping;
    }

    void Engine::FSequencePlayer::SetRestoreOnFinish(bool RestoreOnFinish) {
        mRestoreOnFinish = RestoreOnFinish;
    }

    Engine::ESequencePlayState Engine::FSequencePlayer::GetState() const {
        return mState;
    }

    double Engine::FSequencePlayer::GetTick() const {
        return mTick;
    }

    void Engine::FSequencePlayer::Evaluate() {
        for (FResolvedBinding& Binding : mBindings) {
            auto* Component{CoreUObject::Cast<Engine::USceneComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Binding.mComponent))};

            if (Component == nullptr) {
                continue;
            }

            for (const auto& Track : Binding.mBinding->GetTracks()) {
                if (Track->GetType() == Asset::ESequenceTrackType::Property) {
                    const auto* PropertyTrack{static_cast<const Asset::FSequencePropertyTrack*>(Track.get())};
                    const CoreUObject::FProperty* Property{Component->GetClass()->FindClassProperty(PropertyTrack->GetPropertyName())};
                    float Value{};

                    if (Property == nullptr || Property->GetType() != CoreUObject::EPropertyType::Float || !PropertyTrack->GetChannel().Evaluate(mTick, Value)) {
                        continue;
                    }

                    const bool Captured{std::any_of(Binding.mBaseProperties.begin(), Binding.mBaseProperties.end(), [&Property](const FSavedProperty& Saved) {
                        return Saved.mName == Property->GetName();
                    })};

                    if (!Captured) {
                        FSavedProperty Saved{};
                        Saved.mName = Property->GetName();
                        Saved.mValue = std::get<float>(Property->ReadValue(Component));
                        Binding.mBaseProperties.push_back(Saved);
                    }

                    Property->WriteValue(Component, CoreUObject::FPropertyValue{Value});
                    continue;
                }

                const Asset::FSequenceTransformSection* Section{static_cast<const Asset::FSequenceTransformTrack*>(Track.get())->FindSection(mTick)};

                if (Section == nullptr) {
                    continue;
                }

                if (!Binding.mCaptured) {
                    Binding.mBaseTransform = Component->GetTransform();
                    Binding.mCaptured = true;
                }

                Component->SetTransform(Section->Evaluate(mTick, Binding.mBaseTransform));
            }
        }

        EvaluateCameraCut();
    }

    void Engine::FSequencePlayer::EvaluateCameraCut() {
        const Asset::FSequenceCameraCut* Cut{mSequence != nullptr ? mSequence->FindCameraCut(mTick) : nullptr};
        auto* Camera{Cut != nullptr ? CoreUObject::Cast<Engine::UCameraComponent>(FindBoundComponent(Cut->mCameraBinding)) : nullptr};

        if (Camera == nullptr || mWorld == nullptr) {
            return;
        }

        if (!mCameraCaptured) {
            const Engine::UCameraComponent* Active{mWorld->GetActiveCamera()};
            mBaseCamera = Active != nullptr ? Active->GetHandle() : CoreUObject::FObjectHandle{};
            mCameraCaptured = true;
        }

        mWorld->SetActiveCamera(Camera);
        mCutCamera = Camera->GetHandle();
    }

}
