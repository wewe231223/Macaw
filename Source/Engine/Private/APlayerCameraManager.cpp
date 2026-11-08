#include "Engine/APlayerCameraManager.h"
#include "Engine/UWorld.h"
#include "Engine/Component/UCameraComponent.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"
#include <algorithm>
#include <cmath>

namespace Engine {

    namespace {
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
        if (Target != nullptr && ResolveTarget(Target->GetHandle()) == nullptr) {
            return false;
        }

        mBlending = false;
        mPendingViewTarget = {};
        mViewTarget = Target != nullptr ? Target->GetHandle() : CoreUObject::FObjectHandle{};

        return true;
    }

    bool Engine::APlayerCameraManager::SetViewTargetWithBlend(Engine::AActor* Target, float BlendTime, Engine::EViewTargetBlendFunction Function) {
        if (Target == nullptr || !std::isfinite(BlendTime) || BlendTime <= 0.0f || !mHasCache) {
            return SetViewTarget(Target);
        }

        if (ResolveTarget(Target->GetHandle()) == nullptr) {
            return false;
        }

        if (Target == GetViewTarget()) {
            return true;
        }

        // 블렌드 도중에 다시 바꾸면 지금 보이는 시점에서 출발합니다.
        if (mBlending) {
            mViewTarget = {};
        }

        mBlendFrom = mCache;
        mPendingViewTarget = Target->GetHandle();
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

                mCache = mBlendFrom.Blend(To, EvaluateBlend(mBlendFunction, Alpha));
                mHasCache = true;

                if (Alpha >= 1.0f) {
                    FinishBlend();
                }

                return;
            }

            FinishBlend();
        }

        const Engine::AActor* Target{ResolveTarget(mViewTarget)};

        mHasCache = Target != nullptr && CalcViewTargetPOV(Target, mCache);
    }

    bool Engine::APlayerCameraManager::HasCameraCache() const {
        return mHasCache;
    }

    const Engine::FMinimalViewInfo& Engine::APlayerCameraManager::GetCameraCacheView() const {
        return mCache;
    }

    Engine::AActor* Engine::APlayerCameraManager::ResolveTarget(CoreUObject::FObjectHandle Handle) const {
        auto* Actor{CoreUObject::Cast<Engine::AActor>(CoreUObject::FObjectRegistry::Get()->Resolve(Handle))};

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

}
