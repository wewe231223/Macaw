#include "pch.h"
#include "World/Component/ULightComponent.h"
#include "World/AActor.h"
#include "World/UWorld.h"

void ULightComponent::BuildLightShaderParameters(FLightShaderParameters& OutParameters) const {
    OutParameters = FLightShaderParameters{.mColor = GetLightColor(), .mIntensity = GetIntensity(), .mPosition = GetComponentLocation(), .mDirection = GetComponentTransform().ToMatrixNoScale().Forward(), .mType = GetLightType()};
}

void ULightComponent::OnRegister() {
    ULightComponentBase::OnRegister();

    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetRenderSubsystem().RegisterComponent(this);
    }
}

void ULightComponent::OnUnregister() {
    AActor* Owner{GetOwner()};

    if (Owner != nullptr && Owner->GetWorld() != nullptr) {
        Owner->GetWorld()->GetRenderSubsystem().UnregisterComponent(this);
    }

    ULightComponentBase::OnUnregister();
}

bool ULightComponent::ShouldCreateRenderState() const {
    return IsVisible();
}

std::unique_ptr<FLightSceneProxy> ULightComponent::CreateSceneProxy() const {
    if (!IsRegistered() || !IsVisible()) {
        return nullptr;
    }

    FLightShaderParameters Parameters{};

    BuildLightShaderParameters(Parameters);

    return std::make_unique<FLightSceneProxy>(GetHandle(), Parameters);
}

void ULightComponent::CreateRenderState() {
    UWorld* World{GetBelongingWorld()};

    if (World == nullptr || !IsRegistered() || IsRenderStateCreated() || !ShouldCreateRenderState()) {
        return;
    }

    std::unique_ptr<FLightSceneProxy> Proxy{CreateSceneProxy()};

    if (Proxy != nullptr) {
        World->GetRenderSubsystem().AddLight(std::move(Proxy));
        UActorComponent::CreateRenderState();
    }
}

void ULightComponent::DestroyRenderState() {
    if (GetBelongingWorld() != nullptr && IsRenderStateCreated()) {
        GetBelongingWorld()->GetRenderSubsystem().RemoveLight(GetHandle());
    }

    UActorComponent::DestroyRenderState();
}

void ULightComponent::SendRenderTransform() {
    if (GetBelongingWorld() != nullptr && IsRenderStateCreated()) {
        GetBelongingWorld()->GetRenderSubsystem().UpdateLightTransform(GetHandle(), GetComponentTransform().ToMatrixNoScale());
    }
}

void ULightComponent::OnTransformUpdate() {
    MarkRenderTransformDirty();
}
