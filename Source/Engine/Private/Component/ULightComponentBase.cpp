#include "Engine/Component/ULightComponentBase.h"
#include "Engine/UWorld.h"
#include "Engine/FSceneInterface.h"
#include "Engine/FAssetManager.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Core/Math/FTransform.h"
#include "Core/Math.h"

#include <algorithm>
#include <cmath>

ULightComponentBase::ULightComponentBase() = default;

ULightComponentBase::~ULightComponentBase() {
    UnregisterComponent();
}

float ULightComponentBase::GetIntensity() const {
    return mIntensity;
}

bool ULightComponentBase::SetIntensity(float Intensity) {
    if (!std::isfinite(Intensity) || Intensity < 0.0f) {
        return false;
    }
    mIntensity = Intensity;
    SynchronizeLight();
    return true;
}

const FVector4& ULightComponentBase::GetLightColor() const {
    return mLightColor;
}

bool ULightComponentBase::SetLightColor(const FVector4& Color) {
    const auto Values{Color.ToArray()};
    if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
        return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
    })) {
        return false;
    }
    mLightColor = Color;
    SynchronizeLight();
    return true;
}

bool ULightComponentBase::IsVisible() const {
    return mVisible;
}

bool ULightComponentBase::SetVisible(bool Visible) {
    mVisible = Visible;
    SynchronizeLight();
    return true;
}

FLightSceneProxy ULightComponentBase::CreateLightProxy() const {
    FLightSceneProxy Proxy{};
    Proxy.mId = GetGuid();
    Proxy.mPosition = GetTransform().mLocation;
    Proxy.mDirection = GetTransform().mRotation.ForwardVector();
    Proxy.mColor = mLightColor;
    Proxy.mIntensity = mIntensity;
    Proxy.mVisible = mVisible;
    Proxy.mIcon = mIcon;
    return Proxy;
}

void ULightComponentBase::SynchronizeLight() {
    if (mIcon == nullptr && GetWorld() != nullptr && GetWorld()->GetAssets() != nullptr) {
        const ELightType Type{CreateLightProxy().mType};
        const char* Name{Type == ELightType::Directional ? "DirectionalLight_64x.png" : Type == ELightType::Ambient ? "SkyLight_64x.png" : Type == ELightType::Spot ? "SpotLight_64x.png" : "PointLight_64x.png"};
        FString Error{};
        mIcon = GetWorld()->GetAssets()->LoadTexture(FString{"Editor/Icon/"} + Name, Error);
    }

    if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
        GetWorld()->GetScene()->AddLight(CreateLightProxy());
    }
}

bool ULightComponentBase::SetTransform(const FTransform& Transform) {
    if (!USceneComponent::SetTransform(Transform)) {
        return false;
    }
    SynchronizeLight();
    return true;
}

void ULightComponentBase::OnRegister() {
    SynchronizeLight();
}

void ULightComponentBase::UnregisterComponent() {
    if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
        GetWorld()->GetScene()->RemoveLight(GetGuid());
    }
    mIcon = nullptr;
    UActorComponent::UnregisterComponent();
}

void ULightComponentBase::DrawVisualization(FPrimitiveDrawInterface& Draw, bool Selected) const {
    const FLightSceneProxy Proxy{CreateLightProxy()};
    const FVector Color{mLightColor.x, mLightColor.y, mLightColor.z};
    const FVector Position{Proxy.mPosition};
    const FVector Forward{Proxy.mDirection};
    const FQuat Rotation{GetTransform().mRotation};
    const FVector Right{Rotation.RightVector()};
    const FVector Up{Rotation.UpVector()};
    if (Proxy.mType == ELightType::Directional || Proxy.mType == ELightType::Spot) {
        const FVector End{Position + Forward * 2.0f};
        Draw.DrawLine(Position, End, Color, true, 2.0f);
        Draw.DrawLine(End, End - Forward * 0.4f + Right * 0.2f, Color, true, 2.0f);
        Draw.DrawLine(End, End - Forward * 0.4f - Right * 0.2f, Color, true, 2.0f);
    }
    if (!Selected || (Proxy.mType != ELightType::Point && Proxy.mType != ELightType::Spot)) {
        return;
    }
    constexpr Uint32 Segments{64};
    const float Radius{Proxy.mAttenuationRadius};
    if (Proxy.mType == ELightType::Point) {
        const std::array<FVector, 3> Axes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
        for (Uint32 Axis{}; Axis < 3; ++Axis) {
            for (Uint32 Index{}; Index < Segments; ++Index) {
                const float Start{Math::ToRadians(360.0f * Index / Segments)};
                const float End{Math::ToRadians(360.0f * (Index + 1) / Segments)};
                Draw.DrawLine(Position + (Axes[Axis] * std::cos(Start) + Axes[(Axis + 1) % 3] * std::sin(Start)) * Radius, Position + (Axes[Axis] * std::cos(End) + Axes[(Axis + 1) % 3] * std::sin(End)) * Radius, Color, true);
            }
        }
        return;
    }
    for (const float Angle : {Proxy.mOuterConeAngle, Proxy.mInnerConeAngle}) {
        const float ConeRadius{Radius * std::sin(Math::ToRadians(Angle))};
        const FVector Center{Position + Forward * (Radius * std::cos(Math::ToRadians(Angle)))};
        const FVector ConeColor{Angle == Proxy.mOuterConeAngle ? Color : Color * 0.5f};
        for (Uint32 Index{}; Index < Segments; ++Index) {
            const float Start{Math::ToRadians(360.0f * Index / Segments)};
            const float End{Math::ToRadians(360.0f * (Index + 1) / Segments)};
            const FVector Edge{Center + (Right * std::cos(Start) + Up * std::sin(Start)) * ConeRadius};
            Draw.DrawLine(Edge, Center + (Right * std::cos(End) + Up * std::sin(End)) * ConeRadius, ConeColor, true);
            if (Index % (Segments / 4) == 0) {
                Draw.DrawLine(Position, Edge, ConeColor, true);
            }
        }
    }
}
