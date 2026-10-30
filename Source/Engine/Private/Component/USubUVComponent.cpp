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

USubUVComponent::USubUVComponent() {
    SetEditorTickable(true);
}

USubUVComponent::~USubUVComponent() {
    ClearTarget();
}

const FString& USubUVComponent::GetTargetName() const {
    return mTargetName;
}

bool USubUVComponent::SetTargetName(const FString& Name) {
    if (Name.size() > 255 || Name.find('\0') != FString::npos) {
        return false;
    }

    ClearTarget();
    mTargetName = Name;
    ApplyFrame();

    return true;
}

UPrimitiveComponent* USubUVComponent::GetTarget() const {
    return Cast<UPrimitiveComponent>(FObjectRegistry::Get()->Resolve(mTarget));
}

Uint32 USubUVComponent::GetColumns() const {
    return mColumns;
}

bool USubUVComponent::SetColumns(Uint32 Columns) {
    if (Columns == 0 || Columns > 256) {
        return false;
    }

    mColumns = Columns;
    mFramePosition = 0.0;
    ApplyFrame();

    return true;
}

Uint32 USubUVComponent::GetRows() const {
    return mRows;
}

bool USubUVComponent::SetRows(Uint32 Rows) {
    if (Rows == 0 || Rows > 256) {
        return false;
    }

    mRows = Rows;
    mFramePosition = 0.0;
    ApplyFrame();

    return true;
}

Uint32 USubUVComponent::GetFrameCount() const {
    return mFrameCount;
}

bool USubUVComponent::SetFrameCount(Uint32 FrameCount) {
    if (FrameCount > 65536) {
        return false;
    }

    mFrameCount = FrameCount;
    mFramePosition = 0.0;
    ApplyFrame();

    return true;
}

float USubUVComponent::GetFramesPerSecond() const {
    return mFramesPerSecond;
}

bool USubUVComponent::SetFramesPerSecond(float FramesPerSecond) {
    if (!std::isfinite(FramesPerSecond) || FramesPerSecond < 0.0f || FramesPerSecond > 10000.0f) {
        return false;
    }

    mFramesPerSecond = FramesPerSecond;

    return true;
}

bool USubUVComponent::IsLooping() const {
    return mLooping;
}

void USubUVComponent::SetLooping(bool Looping) {
    mLooping = Looping;
}

bool USubUVComponent::IsPlaying() const {
    return mPlaying;
}

void USubUVComponent::SetPlaying(bool Playing) {
    mPlaying = Playing;
    ApplyFrame();
}

Uint32 USubUVComponent::GetEffectiveFrameCount() const {
    return mFrameCount == 0 ? mColumns * mRows : std::min(mFrameCount, mColumns * mRows);
}

Uint32 USubUVComponent::GetCurrentFrame() const {
    return std::min(static_cast<Uint32>(mFramePosition), GetEffectiveFrameCount() - 1);
}

void USubUVComponent::Play() {
    if (mFramePosition >= static_cast<double>(GetEffectiveFrameCount())) {
        mFramePosition = 0.0;
    }

    SetPlaying(true);
}

void USubUVComponent::Pause() {
    SetPlaying(false);
}

void USubUVComponent::Stop() {
    mFramePosition = 0.0;
    SetPlaying(false);
}

void USubUVComponent::Tick(float DeltaTime) {
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

void USubUVComponent::OnRegister() {
    ApplyFrame();
}

void USubUVComponent::UnregisterComponent() {
    ClearTarget();
    UActorComponent::UnregisterComponent();
}

void USubUVComponent::ClearTarget() {
    if (UPrimitiveComponent* Target{GetTarget()}) {
        Target->ClearUVTransform(GetHandle());
    }

    mTarget = {};
}

void USubUVComponent::ApplyFrame() {
    if (!IsRegistered() || GetOwner() == nullptr) {
        return;
    }

    UPrimitiveComponent* Target{GetTarget()};

    if (Target == nullptr) {
        if (!mTargetName.empty()) {
            Target = Cast<UPrimitiveComponent>(FObjectRegistry::Get()->FindObject(FName{mTargetName}, GetOwner()));

            if (Target != nullptr && !Target->IsA<UBillboardComponent>() && !Target->IsA<UStaticMeshComponent>()) {
                Target = nullptr;
            }
        } else {
            for (const auto& Component : GetOwner()->GetComponents()) {
                auto* Primitive{Cast<UPrimitiveComponent>(Component.get())};

                if (Primitive != nullptr && (Primitive->IsA<UBillboardComponent>() || Primitive->IsA<UStaticMeshComponent>())) {
                    if (Target == nullptr || Primitive->IsA<UBillboardComponent>()) {
                        Target = Primitive;
                    }

                    if (Primitive->IsA<UBillboardComponent>()) {
                        break;
                    }
                }
            }
        }

        mTarget = Target != nullptr ? Target->GetHandle() : FObjectHandle{};
    }

    if (Target != nullptr) {
        const Uint32 Frame{GetCurrentFrame()};
        Target->SetUVTransform({1.0f / mColumns, 1.0f / mRows}, {static_cast<float>(Frame % mColumns) / mColumns, static_cast<float>(Frame / mColumns) / mRows}, GetHandle());
    }
}
