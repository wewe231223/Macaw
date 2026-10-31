/**
 * @file ULightComponentBase.cpp
 * @brief 광원의 색상·밝기·표시 상태와 렌더 장면 등록을 관리합니다.
 * @ingroup Engine
 */

#include "Engine/Component/ULightComponentBase.h"
#include "Engine/UWorld.h"
#include "Engine/FSceneInterface.h"
#include "Asset/FAssetManager.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Core/Math/FTransform.h"
#include "Core/Math.h"
#include "Asset/UTexture2D.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Literals.h"

#include <Windows.h>
#include <algorithm>
#include <cmath>

using namespace Core::Literals;

namespace Engine {

    Engine::ULightComponentBase::ULightComponentBase() = default;

    Engine::ULightComponentBase::~ULightComponentBase() {
        UnregisterComponent();
    }

    float Engine::ULightComponentBase::GetIntensity() const {
        return mIntensity;
    }

    bool Engine::ULightComponentBase::SetIntensity(float Intensity) {
        if (!std::isfinite(Intensity) || Intensity < 0.0f) {
            return false;
        }

        mIntensity = Intensity;
        SynchronizeLight();

        return true;
    }

    const Core::FVector4& Engine::ULightComponentBase::GetLightColor() const {
        return mLightColor;
    }

    bool Engine::ULightComponentBase::SetLightColor(const Core::FVector4& Color) {
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

    bool Engine::ULightComponentBase::IsVisible() const {
        return mVisible;
    }

    bool Engine::ULightComponentBase::SetVisible(bool Visible) {
        mVisible = Visible;
        SynchronizeLight();
        return true;
    }

    Engine::FLightSceneProxy Engine::ULightComponentBase::CreateLightProxy() const {
        Engine::FLightSceneProxy Proxy{};
        Proxy.mId = GetGuid();
        Proxy.mPosition = GetTransform().mLocation;
        Proxy.mDirection = GetTransform().mRotation.ForwardVector();
        Proxy.mColor = mLightColor;
        Proxy.mIntensity = mIntensity;
        Proxy.mVisible = mVisible;
        Proxy.mIcon = mIcon;

        return Proxy;
    }

    void Engine::ULightComponentBase::SynchronizeLight() {
        Asset::FAssetManager* Assets{GetWorld() != nullptr ? GetWorld()->GetAssets() : nullptr};

        if (mIconAssets != Assets) {
            mIcon = nullptr;
            mFallbackIcon.reset();
            mIconAssets = Assets;
        }

        if (mIcon == nullptr && GetWorld() != nullptr) {
            ResolveIcon(Assets);
        }

        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->AddLight(CreateLightProxy());
        }
    }

    bool Engine::ULightComponentBase::SetTransform(const Core::FTransform& Transform) {
        if (!Engine::USceneComponent::SetTransform(Transform)) {
            return false;
        }

        SynchronizeLight();

        return true;
    }

    void Engine::ULightComponentBase::OnRegister() {
        SynchronizeLight();
    }

    void Engine::ULightComponentBase::UnregisterComponent() {
        if (GetWorld() != nullptr && GetWorld()->GetScene() != nullptr) {
            GetWorld()->GetScene()->RemoveLight(GetGuid());
        }

        mIcon = nullptr;
        mIconAssets = nullptr;
        mFallbackIcon.reset();
        Engine::UActorComponent::UnregisterComponent();
    }

    void Engine::ULightComponentBase::ResolveIcon(Asset::FAssetManager* Assets) {
        const Engine::ELightType Type{CreateLightProxy().mType};
        const char* Name{Type == Engine::ELightType::Directional ? "DirectionalLight_64x.png" : Type == Engine::ELightType::Ambient ? "SkyLight_64x.png" : Type == Engine::ELightType::Spot ? "SpotLight_64x.png" : "PointLight_64x.png"};

        if (Assets != nullptr) {
            Core::FString Error{};
            mIcon = Assets->LoadTexture(Core::FString{"Editor/Icon/"} + Name, Error);
        }

        if (mIcon != nullptr) {
            return;
        }

        std::array<wchar_t, 32768> Executable{};
        const DWORD Length{GetModuleFileNameW(nullptr, Executable.data(), static_cast<DWORD>(Executable.size()))};
        std::filesystem::path Path{Length > 0 && Length < Executable.size() ? std::filesystem::path{Executable.data()}.parent_path() / "Templates/Project/Content/Editor/Icon" / Name : std::filesystem::path{}};
        std::error_code Error{};

        if (Path.empty() || !std::filesystem::is_regular_file(Path, Error)) {
            Path = std::filesystem::path{"Templates/Project/Content/Editor/Icon"} / Name;
        }

        auto Icon{CoreUObject::NewObject<Asset::UTexture2D>()};

        if (Icon->LoadFromFile(Path, true)) {
            mFallbackIcon = std::move(Icon);
            mIcon = mFallbackIcon.get();
        }
    }

    void Engine::ULightComponentBase::DrawVisualization(Engine::FPrimitiveDrawInterface& Draw, bool Selected) const {
        const Engine::FLightSceneProxy Proxy{CreateLightProxy()};
        const Core::FVector Color{mLightColor.x, mLightColor.y, mLightColor.z};
        const Core::FVector Position{Proxy.mPosition};
        const Core::FVector Forward{Proxy.mDirection};
        const Core::FQuat Rotation{GetTransform().mRotation};
        const Core::FVector Right{Rotation.RightVector()};
        const Core::FVector Up{Rotation.UpVector()};

        if (Proxy.mType == Engine::ELightType::Directional || Proxy.mType == Engine::ELightType::Spot) {
            const Core::FVector End{Position + Forward * 2.0f};
            Draw.DrawLine(Position, End, Color, true, 2.0f);
            Draw.DrawLine(End, End - Forward * 0.4f + Right * 0.2f, Color, true, 2.0f);
            Draw.DrawLine(End, End - Forward * 0.4f - Right * 0.2f, Color, true, 2.0f);
        }

        if (!Selected || (Proxy.mType != Engine::ELightType::Point && Proxy.mType != Engine::ELightType::Spot)) {
            return;
        }

        constexpr Core::Uint32 Segments{64};
        const float Radius{Proxy.mAttenuationRadius};

        if (Proxy.mType == Engine::ELightType::Point) {
            const std::array<Core::FVector, 3> Axes{{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

            for (Core::Uint32 Axis{}; Axis < 3; ++Axis) {
                for (Core::Uint32 Index{}; Index < Segments; ++Index) {
                    const Core::FAngle Start{1_turn * Index / Segments};
                    const Core::FAngle End{1_turn * (Index + 1) / Segments};
                    Draw.DrawLine(Position + (Axes[Axis] * Core::Math::Cos(Start) + Axes[(Axis + 1) % 3] * Core::Math::Sin(Start)) * Radius, Position + (Axes[Axis] * Core::Math::Cos(End) + Axes[(Axis + 1) % 3] * Core::Math::Sin(End)) * Radius, Color, true);
                }
            }

            return;
        }

        for (const float Angle : {Proxy.mOuterConeAngle, Proxy.mInnerConeAngle}) {
            const float ConeRadius{Radius * Core::Math::Sin(Angle * 1_deg)};
            const Core::FVector Center{Position + Forward * (Radius * Core::Math::Cos(Angle * 1_deg))};
            const Core::FVector ConeColor{Angle == Proxy.mOuterConeAngle ? Color : Color * 0.5f};

            for (Core::Uint32 Index{}; Index < Segments; ++Index) {
                const Core::FAngle Start{1_turn * Index / Segments};
                const Core::FAngle End{1_turn * (Index + 1) / Segments};
                const Core::FVector Edge{Center + (Right * Core::Math::Cos(Start) + Up * Core::Math::Sin(Start)) * ConeRadius};
                Draw.DrawLine(Edge, Center + (Right * Core::Math::Cos(End) + Up * Core::Math::Sin(End)) * ConeRadius, ConeColor, true);

                if (Index % (Segments / 4) == 0) {
                    Draw.DrawLine(Position, Edge, ConeColor, true);
                }
            }
        }
    }

}
