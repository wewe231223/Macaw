/**
 * @file FSequencePlayer.cpp
 * @brief 시퀀스의 시간을 진행하고 월드 객체에 값을 적용하는 플레이어에 대한 선언입니다.
 * @ingroup Engine
 */

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

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 월드에서 GUID가 일치하는 공간 컴포넌트를 찾습니다.
         * @param[in] World 컴포넌트를 찾을 월드입니다.
         * @param[in] Guid 찾을 공간 컴포넌트의 장면 GUID입니다.
         * @return GUID가 일치하는 공간 컴포넌트이며 없으면 nullptr입니다.
         */
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

    /**
     * @brief 시퀀스의 바인딩을 월드 객체로 해석하고 정지 상태로 초기화합니다.
     */
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

            const Engine::USceneComponent* Component{Target != Targets.end() ? FindSceneComponent(World, Target->mObject.GetGuid()) : nullptr};

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

    /**
     * @brief 시퀀스 연결과 보관한 변환을 값 복원 없이 해제합니다.
     */
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

    /**
     * @brief 재생을 시작하거나 일시정지를 해제하고 현재 시간의 값을 적용합니다.
     */
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

    /**
     * @brief 재생 중이면 현재 시간에서 일시정지합니다.
     */
    void Engine::FSequencePlayer::Pause() {
        if (mState == Engine::ESequencePlayState::Playing) {
            mState = Engine::ESequencePlayState::Paused;
        }
    }

    /**
     * @brief 재생을 끝내고 복원 설정이 켜져 있으면 원래 변환을 복원합니다.
     */
    void Engine::FSequencePlayer::Stop() {
        mState = Engine::ESequencePlayState::Stopped;

        if (mRestoreOnFinish) {
            RestoreState();
        }
    }

    /**
     * @brief 재생 중이면 시간을 진행하고 값을 적용합니다.
     */
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

    /**
     * @brief 재생 구간 안의 시간으로 이동하여 값을 적용합니다.
     */
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

    /**
     * @brief 바인딩에 연결된 공간 컴포넌트를 조회합니다.
     */
    Engine::USceneComponent* Engine::FSequencePlayer::FindBoundComponent(const Core::FGuid& BindingId) const {
        for (const FResolvedBinding& Binding : mBindings) {
            if (Binding.mBinding->GetId() == BindingId) {
                return CoreUObject::Cast<Engine::USceneComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(Binding.mComponent));
            }
        }

        return nullptr;
    }

    /**
     * @brief 값을 쓴 객체와 활성 카메라를 원래 상태로 되돌립니다.
     */
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

    /**
     * @brief 현재 카메라 컷이 활성화한 카메라를 조회합니다.
     */
    Engine::UCameraComponent* Engine::FSequencePlayer::GetCutCamera() const {
        return CoreUObject::Cast<Engine::UCameraComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mCutCamera));
    }

    /**
     * @brief 끝에 도달했을 때 시작 시간으로 돌아갈지 설정합니다.
     */
    void Engine::FSequencePlayer::SetLooping(bool Looping) {
        mLooping = Looping;
    }

    /**
     * @brief 재생이 끝났을 때 원래 변환을 복원할지 설정합니다.
     */
    void Engine::FSequencePlayer::SetRestoreOnFinish(bool RestoreOnFinish) {
        mRestoreOnFinish = RestoreOnFinish;
    }

    /**
     * @brief 현재 재생 상태를 반환합니다.
     */
    Engine::ESequencePlayState Engine::FSequencePlayer::GetState() const {
        return mState;
    }

    /**
     * @brief 현재 재생 시간을 반환합니다.
     */
    double Engine::FSequencePlayer::GetTick() const {
        return mTick;
    }

    /**
     * @brief 현재 시간의 값을 바인딩 대상에 적용합니다.
     */
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

    /**
     * @brief 현재 시간의 카메라 컷을 월드의 활성 카메라에 적용합니다.
     */
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
