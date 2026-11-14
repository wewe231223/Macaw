/**
 * @file APlayerCameraManager.cpp
 * @brief 뷰 타깃에서 매 프레임 카메라 시점을 계산하고 캐시하는 카메라 관리자입니다.
 * @ingroup Engine
 */

#include "Engine/APlayerCameraManager.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/UCameraShakeModifier.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include <algorithm>
#include <cmath>

namespace Engine {

    /**
     * @brief 뷰 타깃에서 매 프레임 카메라 시점을 계산하고 캐시하는 카메라 관리자의 파일 내부 보조 구현입니다.
     */
    namespace {

        /**
         * @brief 뷰 타깃 전환 방식에 맞게 정규화된 블렌드 가중치를 계산합니다.
         * @param Function 뷰 타깃 전환 가중치 함수 종류입니다.
         * @param Alpha 0~1의 보간 가중치입니다.
         * @return 전환 곡선을 적용한 가중치입니다.
         */
        float EvaluateBlend(Engine::EViewTargetBlendFunction Function, float Alpha) {
            switch (Function) {
                case Engine::EViewTargetBlendFunction::Cubic:
                    return Alpha * Alpha * (3.0f - 2.0f * Alpha);
                case Engine::EViewTargetBlendFunction::EaseIn:
                    return Alpha * Alpha;
                case Engine::EViewTargetBlendFunction::EaseOut:
                    return 1.0f - (1.0f - Alpha) * (1.0f - Alpha);
                case Engine::EViewTargetBlendFunction::Linear:
                default:
                    return Alpha;
            }
        }
    }

    Engine::APlayerCameraManager::APlayerCameraManager() = default;

    Engine::APlayerCameraManager::~APlayerCameraManager() = default;

    Engine::AActor* Engine::APlayerCameraManager::GetViewTarget() const {
        return ResolveTarget(mBlending ? mPendingViewTarget : mViewTarget);
    }

    bool Engine::APlayerCameraManager::SetViewTarget(Engine::AActor* Target) {
        if (Target != nullptr && ResolveTarget(CoreUObject::MakeObjectRef(Target)) == nullptr) {
            return false;
        }

        mBlending = false;
        mPendingViewTarget = {};
        mViewTarget = Target;

        return true;
    }

    bool Engine::APlayerCameraManager::SetViewTargetWithBlend(Engine::AActor* Target, float BlendTime, Engine::EViewTargetBlendFunction Function) {
        if (Target == nullptr || !std::isfinite(BlendTime) || BlendTime <= 0.0f || !mHasCache) {
            return SetViewTarget(Target);
        }

        if (ResolveTarget(CoreUObject::MakeObjectRef(Target)) == nullptr) {
            return false;
        }

        if (Target == GetViewTarget()) {
            return true;
        }

        // 블렌드 도중에 다시 바꾸면 지금 보이는 시점에서 출발합니다.
        if (mBlending) {
            mViewTarget = {};
        }

        mBlendFrom = mBaseView;
        mPendingViewTarget = CoreUObject::MakeObjectRef(Target);
        mBlendTime = BlendTime;
        mBlendElapsed = 0.0f;
        mBlendFunction = Function;
        mBlending = true;

        return true;
    }

    bool Engine::APlayerCameraManager::IsBlending() const {
        return mBlending;
    }

    Core::FVector Engine::APlayerCameraManager::GetCameraLocation() const {
        return mCache.mLocation;
    }

    Core::FQuat Engine::APlayerCameraManager::GetCameraRotation() const {
        return mCache.mRotation;
    }

    float Engine::APlayerCameraManager::GetFieldOfView() const {
        return mCache.mFieldOfView;
    }

    void Engine::APlayerCameraManager::UpdateCamera(float DeltaTime) {
        if (!std::isfinite(DeltaTime) || DeltaTime < 0.0f) {
            DeltaTime = 0.0f;
        }

        if (mBlending) {
            const Engine::AActor* Pending{ResolveTarget(mPendingViewTarget)};
            Engine::FMinimalViewInfo To{};

            if (Pending != nullptr && CalcViewTargetPOV(Pending, To)) {
                // 출발 대상이 살아 있으면 계속 따라가고, 사라졌으면 마지막 시점에서 출발합니다.
                if (const Engine::AActor* From{ResolveTarget(mViewTarget)}) {
                    CalcViewTargetPOV(From, mBlendFrom);
                }

                mBlendElapsed += DeltaTime;

                const float Alpha{std::clamp(mBlendElapsed / mBlendTime, 0.0f, 1.0f)};

                mBaseView = mBlendFrom.Blend(To, EvaluateBlend(mBlendFunction, Alpha));

                if (Alpha >= 1.0f) {
                    FinishBlend();
                }

                FinishUpdate(DeltaTime, true);

                return;
            }

            FinishBlend();
        }

        const Engine::AActor* Target{ResolveTarget(mViewTarget)};

        FinishUpdate(DeltaTime, Target != nullptr && CalcViewTargetPOV(Target, mBaseView));
    }

    void Engine::APlayerCameraManager::FinishUpdate(float DeltaTime, bool HasBaseView) {
        mHasCache = HasBaseView;

        if (mHasCache) {
            mCache = mBaseView;
            ApplyCameraModifiers(DeltaTime, mCache);
        }
    }

    bool Engine::APlayerCameraManager::HasCameraCache() const {
        return mHasCache;
    }

    const Engine::FMinimalViewInfo& Engine::APlayerCameraManager::GetCameraCacheView() const {
        return mCache;
    }

    Engine::AActor* Engine::APlayerCameraManager::ResolveTarget(CoreUObject::TObjectRef<Engine::AActor> Handle) const {
        auto* Actor{CoreUObject::Cast<Engine::AActor>(Handle.Get())};

        return Actor != nullptr && !Actor->IsPendingDestroy() && GetWorld() != nullptr && Actor->GetWorld() == GetWorld() ? Actor : nullptr;
    }

    bool Engine::APlayerCameraManager::CalcViewTargetPOV(const Engine::AActor* Target, Engine::FMinimalViewInfo& ViewInfo) const {
        for (const auto& Component : Target->GetComponents()) {
            if (const auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(Component.get())}) {
                if (!Camera->GetTransform().IsValid()) {
                    return false;
                }

                ViewInfo = Camera->GetCameraViewInfo();

                return true;
            }
        }

        const Core::FTransform Transform{Target->GetActorTransform()};

        if (!Transform.IsValid()) {
            return false;
        }

        ViewInfo.mLocation = Transform.mLocation;
        ViewInfo.mRotation = Transform.mRotation;
        ViewInfo.mFieldOfView = mDefaultFieldOfView;

        return true;
    }

    void Engine::APlayerCameraManager::FinishBlend() {
        if (ResolveTarget(mPendingViewTarget) != nullptr) {
            mViewTarget = mPendingViewTarget;
        }

        mPendingViewTarget = {};
        mBlending = false;
    }

    bool Engine::APlayerCameraManager::InitializeDefaultComponents() {
        if (!Engine::AActor::InitializeDefaultComponents()) {
            return false;
        }

        if (mCameraShake == nullptr) {
            auto Shake{CoreUObject::NewObject<Engine::UCameraShakeModifier>()};
            Shake->SetName("CameraShakeModifier");
            mCameraShake = static_cast<Engine::UCameraShakeModifier*>(AddCameraModifier(std::move(Shake)));
        }

        return mCameraShake != nullptr;
    }

    Engine::UCameraModifier* Engine::APlayerCameraManager::AddCameraModifier(std::unique_ptr<Engine::UCameraModifier> Modifier) {
        if (Modifier == nullptr || Modifier->GetCameraOwner() != nullptr || !Modifier->AddedToCamera(this)) {
            return nullptr;
        }

        Engine::UCameraModifier* Added{Modifier.get()};

        mModifiers.push_back(std::move(Modifier));

        return Added;
    }

    Engine::UCameraModifier* Engine::APlayerCameraManager::AddNewCameraModifier(const CoreUObject::UClass* Class) {
        if (Class == nullptr || !Class->IsChildOf(Engine::UCameraModifier::StaticClass()) || !Class->CanCreate()) {
            return nullptr;
        }

        std::unique_ptr<CoreUObject::UObject> Object{Class->CreateInstance()};

        if (CoreUObject::Cast<Engine::UCameraModifier>(Object.get()) == nullptr) {
            return nullptr;
        }

        return AddCameraModifier(std::unique_ptr<Engine::UCameraModifier>{static_cast<Engine::UCameraModifier*>(Object.release())});
    }

    bool Engine::APlayerCameraManager::RemoveCameraModifier(Engine::UCameraModifier* Modifier) {
        const auto Found{std::find_if(mModifiers.begin(), mModifiers.end(), [Modifier](const auto& Entry) {
            return Entry.get() == Modifier;
        })};

        if (Modifier == nullptr || Found == mModifiers.end()) {
            return false;
        }

        if (Modifier == mCameraShake) {
            mCameraShake = nullptr;
        }

        // 적용 중인 모디파이어가 자신을 빼도 해제는 이번 갱신이 끝난 뒤에 합니다.
        mRemovedModifiers.push_back(std::move(*Found));
        mModifiers.erase(Found);

        return true;
    }

    Engine::UCameraModifier* Engine::APlayerCameraManager::FindCameraModifierByClass(const CoreUObject::UClass* Class) const {
        for (const auto& Modifier : mModifiers) {
            if (Class != nullptr && Modifier->GetClass()->IsChildOf(Class)) {
                return Modifier.get();
            }
        }

        return nullptr;
    }

    Engine::UCameraShakeModifier* Engine::APlayerCameraManager::GetCameraShakeModifier() const {
        return mCameraShake;
    }

    void Engine::APlayerCameraManager::ApplyCameraModifiers(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo) {
        std::stable_sort(mModifiers.begin(), mModifiers.end(), [](const auto& Left, const auto& Right) {
            return Left->GetPriority() < Right->GetPriority();
        });

        Core::TArray<Engine::UCameraModifier*> Snapshot{};
        Snapshot.reserve(mModifiers.size());

        for (const auto& Modifier : mModifiers) {
            Snapshot.push_back(Modifier.get());
        }

        for (Engine::UCameraModifier* Modifier : Snapshot) {
            // 앞 순서의 모디파이어가 빼낸 모디파이어는 건너뜁니다.
            const bool Present{std::any_of(mModifiers.begin(), mModifiers.end(), [Modifier](const auto& Entry) {
                return Entry.get() == Modifier;
            })};

            if (!Present || Modifier->IsDisabled()) {
                continue;
            }

            Modifier->UpdateAlpha(DeltaTime);

            if (!Modifier->IsDisabled() && Modifier->ModifyCamera(DeltaTime, ViewInfo)) {
                break;
            }
        }

        mRemovedModifiers.clear();
    }

}
