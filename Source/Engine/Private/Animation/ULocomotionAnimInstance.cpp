#include "Engine/Animation/ULocomotionAnimInstance.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

namespace Engine {

    namespace {
        bool IsAssetPath(const Core::FString& Path) {
            const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

            if (Path.find('\0') != Core::FString::npos || (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset"))) {
                return false;
            }

            return std::none_of(AssetPath.begin(), AssetPath.end(), [](const std::filesystem::path& Part) {
                return Part == L"..";
            });
        }
    }

    ULocomotionAnimInstance::ULocomotionAnimInstance() = default;

    ULocomotionAnimInstance::~ULocomotionAnimInstance() = default;

    const Core::FString& ULocomotionAnimInstance::GetLocomotionBlendSpace() const {
        return mLocomotionBlendSpace;
    }

    bool ULocomotionAnimInstance::SetLocomotionBlendSpace(const Core::FString& Path) {
        if (!IsAssetPath(Path)) {
            return false;
        }

        mLocomotionBlendSpace = Path;

        return true;
    }

    const Core::FString& ULocomotionAnimInstance::GetOverlayAnimation() const {
        return mOverlayAnimation;
    }

    bool ULocomotionAnimInstance::SetOverlayAnimation(const Core::FString& Path) {
        if (!IsAssetPath(Path)) {
            return false;
        }

        mOverlayAnimation = Path;

        return true;
    }

    float ULocomotionAnimInstance::GetBlendAlpha() const {
        return mBlendAlpha;
    }

    void ULocomotionAnimInstance::SetBlendAlpha(float Alpha) {
        mBlendAlpha = std::isfinite(Alpha) ? std::clamp(Alpha, 0.0f, 1.0f) : 0.0f;
    }

    float ULocomotionAnimInstance::GetManualSpeed() const {
        return mManualSpeed;
    }

    void ULocomotionAnimInstance::SetManualSpeed(float Speed) {
        mManualSpeed = std::isfinite(Speed) ? std::max(Speed, 0.0f) : 0.0f;
    }

    float ULocomotionAnimInstance::GetSpeed() const {
        return mSpeed;
    }

    void ULocomotionAnimInstance::NativeInitializeAnimation() {
        mLocomotionPlayer.mName = "Locomotion";
        mLocomotionPlayer.mBlendSpace = LoadBlendSpace(mLocomotionBlendSpace);
        mLocomotionPlayer.mXBinding = &mSpeed;
        mLocomotionPlayer.mXBindingName = "Speed";

        mOverlayPlayer.mName = "Overlay";
        mOverlayPlayer.mSequence = LoadAnimSequence(mOverlayAnimation);

        mOverlayBlend.mName = "Overlay Blend";
        mOverlayBlend.mA.mLinkedNode = &mLocomotionPlayer;
        mOverlayBlend.mB.mLinkedNode = &mOverlayPlayer;
        mOverlayBlend.mAlphaBinding = &mBlendAlpha;
        mOverlayBlend.mAlphaBindingName = "BlendAlpha";

        // 덧섞을 시퀀스가 없으면 Two-Way Blend 없이 로코모션을 바로 출력합니다.
        mRootNode = mOverlayAnimation.empty() ? static_cast<Engine::FAnimNode_Base*>(&mLocomotionPlayer) : static_cast<Engine::FAnimNode_Base*>(&mOverlayBlend);
    }

    void ULocomotionAnimInstance::NativeUpdateAnimation(float) {
        // 언리얼 예제의 Pawn->GetVelocity().Size2D()에 해당합니다. Z가 위쪽이므로 XY 평면의 속력을 씁니다.
        const Core::FVector& Velocity{GetOwnerVelocity()};

        mSpeed = mUseManualSpeed ? mManualSpeed : std::sqrt(Velocity.x * Velocity.x + Velocity.y * Velocity.y);
    }

}
