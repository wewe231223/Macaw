#include "Engine/UCameraShakeModifier.h"
#include "Engine/FMinimalViewInfo.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FRotator.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Engine {

    Engine::UCameraShakeModifier::UCameraShakeModifier() = default;

    Engine::UCameraShakeModifier::~UCameraShakeModifier() = default;

    bool Engine::UCameraShakeModifier::ModifyCamera(float DeltaTime, Engine::FMinimalViewInfo& ViewInfo) {
        constexpr float TwoPi{2.0f * std::numbers::pi_v<float>};
        Core::FVector Location{};
        Core::FVector Rotation{};
        float FieldOfView{};

        for (FActiveShake& Shake : mShakes) {
            Shake.mElapsed += DeltaTime;

            const float Weight{EvaluateWeight(Shake) * GetAlpha()};

            if (Weight <= 0.0f) {
                continue;
            }

            float Wave[7]{};

            for (int Axis{}; Axis < 7; ++Axis) {
                Wave[Axis] = std::sin(TwoPi * Shake.mFrequency * Shake.mRate[Axis] * Shake.mElapsed + Shake.mPhase[Axis]) * Weight;
            }

            Location.x += Shake.mLocationAmplitude.x * Wave[0];
            Location.y += Shake.mLocationAmplitude.y * Wave[1];
            Location.z += Shake.mLocationAmplitude.z * Wave[2];
            Rotation.x += Shake.mRotationAmplitude.x * Wave[3];
            Rotation.y += Shake.mRotationAmplitude.y * Wave[4];
            Rotation.z += Shake.mRotationAmplitude.z * Wave[5];
            FieldOfView += Shake.mFieldOfViewAmplitude * Wave[6];
        }

        std::erase_if(mShakes, [](const FActiveShake& Shake) {
            return EvaluateWeight(Shake) < 0.0f;
        });

        // 위치와 회전 모두 카메라 기준으로 더해 어느 방향을 보든 같은 느낌으로 흔들립니다.
        ViewInfo.mLocation = ViewInfo.mLocation + ViewInfo.mRotation.RotateVector(Location);
        ViewInfo.mRotation = (ViewInfo.mRotation * Core::FRotator{Rotation.x, Rotation.y, Rotation.z}.Quaternion()).Normalized();
        ViewInfo.mFieldOfView = std::clamp(ViewInfo.mFieldOfView + FieldOfView, 1.0f, 170.0f);

        return false;
    }

    Core::Int32 Engine::UCameraShakeModifier::StartShake(float Duration, float Frequency, const Core::FVector& LocationAmplitude, const Core::FVector& RotationAmplitude, float FieldOfViewAmplitude, float BlendInTime, float BlendOutTime) {
        if (!std::isfinite(Duration) || !std::isfinite(Frequency) || Frequency <= 0.0f || !std::isfinite(FieldOfViewAmplitude) || !std::isfinite(BlendInTime) || !std::isfinite(BlendOutTime)) {
            return 0;
        }

        std::uniform_real_distribution<float> Phase{0.0f, 2.0f * std::numbers::pi_v<float>};
        std::uniform_real_distribution<float> Rate{0.85f, 1.15f};
        FActiveShake Shake{};

        Shake.mId = mNextId++;
        Shake.mDuration = Duration;
        Shake.mFrequency = Frequency;
        Shake.mLocationAmplitude = LocationAmplitude;
        Shake.mRotationAmplitude = RotationAmplitude;
        Shake.mFieldOfViewAmplitude = FieldOfViewAmplitude;
        Shake.mBlendInTime = std::max(BlendInTime, 0.0f);
        Shake.mBlendOutTime = std::max(BlendOutTime, 0.0f);

        for (int Axis{}; Axis < 7; ++Axis) {
            Shake.mPhase[Axis] = Phase(mRandom);
            Shake.mRate[Axis] = Rate(mRandom);
        }

        mShakes.push_back(Shake);

        return Shake.mId;
    }

    bool Engine::UCameraShakeModifier::StopShake(Core::Int32 Id, bool Immediate) {
        const auto Found{std::find_if(mShakes.begin(), mShakes.end(), [Id](const FActiveShake& Shake) {
            return Shake.mId == Id;
        })};

        if (Found == mShakes.end()) {
            return false;
        }

        if (Immediate || Found->mBlendOutTime <= 0.0f) {
            mShakes.erase(Found);
        } else if (Found->mStopTime < 0.0f) {
            Found->mStopTime = Found->mElapsed;
        }

        return true;
    }

    void Engine::UCameraShakeModifier::StopAllShakes(bool Immediate) {
        if (Immediate) {
            mShakes.clear();

            return;
        }

        for (FActiveShake& Shake : mShakes) {
            if (Shake.mStopTime < 0.0f) {
                Shake.mStopTime = Shake.mElapsed;
            }
        }

        std::erase_if(mShakes, [](const FActiveShake& Shake) {
            return Shake.mBlendOutTime <= 0.0f;
        });
    }

    Core::Int32 Engine::UCameraShakeModifier::GetActiveShakeCount() const {
        return static_cast<Core::Int32>(mShakes.size());
    }

    float Engine::UCameraShakeModifier::EvaluateWeight(const FActiveShake& Shake) {
        float Weight{Shake.mBlendInTime > 0.0f ? std::min(Shake.mElapsed / Shake.mBlendInTime, 1.0f) : 1.0f};

        if (Shake.mStopTime >= 0.0f) {
            const float Since{Shake.mElapsed - Shake.mStopTime};

            if (Since >= Shake.mBlendOutTime) {
                return -1.0f;
            }

            Weight = std::min(Weight, 1.0f - Since / Shake.mBlendOutTime);
        } else if (Shake.mDuration > 0.0f) {
            const float Remaining{Shake.mDuration - Shake.mElapsed};

            if (Remaining <= 0.0f) {
                return -1.0f;
            }

            if (Shake.mBlendOutTime > 0.0f) {
                Weight = std::min(Weight, Remaining / Shake.mBlendOutTime);
            }
        }

        return Weight;
    }

}
