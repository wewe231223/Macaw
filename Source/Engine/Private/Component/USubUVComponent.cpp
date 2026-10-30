/**
 * @file USubUVComponent.cpp
 * @brief 아틀라스 격자 UV의 자동 재생을 제공합니다.
 * @ingroup Engine
 */

#include "Engine/Component/USubUVComponent.h"
#include "Engine/Component/UBillboardComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/AActor.h"
#include "CoreUObject/FObjectRegistry.h"
#include "CoreUObject/Reflection/Cast.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    Engine::USubUVComponent::USubUVComponent() {
        SetEditorTickable(true);
    }

    Engine::USubUVComponent::~USubUVComponent() {
        ClearTarget();
    }

    const Core::FString& Engine::USubUVComponent::GetTargetName() const {
        return mTargetName;
    }

    bool Engine::USubUVComponent::SetTargetName(const Core::FString& Name) {
        if (Name.size() > 255 || Name.find('\0') != Core::FString::npos) {
            return false;
        }

        ClearTarget();
        mTargetName = Name;
        ApplyFrame();

        return true;
    }

    Engine::UPrimitiveComponent* Engine::USubUVComponent::GetTarget() const {
        return CoreUObject::Cast<Engine::UPrimitiveComponent>(CoreUObject::FObjectRegistry::Get()->Resolve(mTarget));
    }

    Core::Uint32 Engine::USubUVComponent::GetColumns() const {
        return mColumns;
    }

    bool Engine::USubUVComponent::SetColumns(Core::Uint32 Columns) {
        if (Columns == 0 || Columns > 256) {
            return false;
        }

        mColumns = Columns;
        mFramePosition = 0.0;
        ApplyFrame();

        return true;
    }

    Core::Uint32 Engine::USubUVComponent::GetRows() const {
        return mRows;
    }

    bool Engine::USubUVComponent::SetRows(Core::Uint32 Rows) {
        if (Rows == 0 || Rows > 256) {
            return false;
        }

        mRows = Rows;
        mFramePosition = 0.0;
        ApplyFrame();

        return true;
    }

    Core::Uint32 Engine::USubUVComponent::GetFrameCount() const {
        return mFrameCount;
    }

    bool Engine::USubUVComponent::SetFrameCount(Core::Uint32 FrameCount) {
        if (FrameCount > 65536) {
            return false;
        }

        mFrameCount = FrameCount;
        mFramePosition = 0.0;
        ApplyFrame();

        return true;
    }

    float Engine::USubUVComponent::GetFramesPerSecond() const {
        return mFramesPerSecond;
    }

    bool Engine::USubUVComponent::SetFramesPerSecond(float FramesPerSecond) {
        if (!std::isfinite(FramesPerSecond) || FramesPerSecond < 0.0f || FramesPerSecond > 10000.0f) {
            return false;
        }

        mFramesPerSecond = FramesPerSecond;

        return true;
    }

    bool Engine::USubUVComponent::IsLooping() const {
        return mLooping;
    }

    void Engine::USubUVComponent::SetLooping(bool Looping) {
        mLooping = Looping;
    }

    bool Engine::USubUVComponent::IsPlaying() const {
        return mPlaying;
    }

    void Engine::USubUVComponent::SetPlaying(bool Playing) {
        mPlaying = Playing;
        ApplyFrame();
    }

    Core::Uint32 Engine::USubUVComponent::GetEffectiveFrameCount() const {
        return mFrameCount == 0 ? mColumns * mRows : std::min(mFrameCount, mColumns * mRows);
    }

    Core::Uint32 Engine::USubUVComponent::GetCurrentFrame() const {
        return std::min(static_cast<Core::Uint32>(mFramePosition), GetEffectiveFrameCount() - 1);
    }

    void Engine::USubUVComponent::Play() {
        if (mFramePosition >= static_cast<double>(GetEffectiveFrameCount())) {
            mFramePosition = 0.0;
        }

        SetPlaying(true);
    }

    void Engine::USubUVComponent::Pause() {
        SetPlaying(false);
    }

    void Engine::USubUVComponent::Stop() {
        mFramePosition = 0.0;
        SetPlaying(false);
    }

    void Engine::USubUVComponent::Tick(float DeltaTime) {
        if (mPlaying && std::isfinite(DeltaTime) && DeltaTime > 0.0f) {
            const double Next{mFramePosition + static_cast<double>(DeltaTime) * mFramesPerSecond};
            const double Count{static_cast<double>(GetEffectiveFrameCount())};
            mFramePosition = mLooping ? std::fmod(Next, Count) : std::min(Next, Count);

            if (!mLooping && Next >= Count) {
                mPlaying = false;
            }
        }

        ApplyFrame();
    }

    void Engine::USubUVComponent::OnRegister() {
        ApplyFrame();
    }

    void Engine::USubUVComponent::UnregisterComponent() {
        ClearTarget();
        Engine::UActorComponent::UnregisterComponent();
    }

    void Engine::USubUVComponent::ClearTarget() {
        if (Engine::UPrimitiveComponent* Target{GetTarget()}) {
            Target->ClearUVTransform(GetHandle());
        }

        mTarget = {};
    }

    void Engine::USubUVComponent::ApplyFrame() {
        if (!IsRegistered() || GetOwner() == nullptr) {
            return;
        }

        Engine::UPrimitiveComponent* Target{GetTarget()};

        if (Target == nullptr) {
            if (!mTargetName.empty()) {
                Target = CoreUObject::Cast<Engine::UPrimitiveComponent>(CoreUObject::FObjectRegistry::Get()->FindObject(Core::FName{mTargetName}, GetOwner()));

                if (Target != nullptr && !Target->IsA<Engine::UBillboardComponent>() && !Target->IsA<Engine::UStaticMeshComponent>()) {
                    Target = nullptr;
                }
            } else {
                for (const auto& Component : GetOwner()->GetComponents()) {
                    auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                    if (Primitive != nullptr && (Primitive->IsA<Engine::UBillboardComponent>() || Primitive->IsA<Engine::UStaticMeshComponent>())) {
                        if (Target == nullptr || Primitive->IsA<Engine::UBillboardComponent>()) {
                            Target = Primitive;
                        }

                        if (Primitive->IsA<Engine::UBillboardComponent>()) {
                            break;
                        }
                    }
                }
            }

            mTarget = Target != nullptr ? Target->GetHandle() : CoreUObject::FObjectHandle{};
        }

        if (Target != nullptr) {
            const Core::Uint32 Frame{GetCurrentFrame()};
            Target->SetUVTransform({1.0f / mColumns, 1.0f / mRows}, {static_cast<float>(Frame % mColumns) / mColumns, static_cast<float>(Frame / mColumns) / mRows}, GetHandle());
        }
    }

}
