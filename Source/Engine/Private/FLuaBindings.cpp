#include "FLuaRuntime.h"

#include "Core/Literals/UnitLiterals.h"
#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Component/UProjectileMovementComponent.h"
#include "Engine/Component/USimpleVehicleMovementComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/Component/UStaticMeshComponent.h"
#include "Engine/Component/UVehicleMovementComponent.h"
#include "Engine/FWorldHit.h"
#include "Engine/UWorld.h"
#include "PhysicsRuntime/FPhysicsRuntime.h"
#include "PhysicsRuntime/FVehicleManager.h"

#include "Asset/FAssetManager.h"
#include "Core/FilePath.h"

#include <PxMaterial.h>
#include <PxPhysics.h>
#include <PxRigidActor.h>
#include <PxShape.h>

#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <tuple>

// 배경 음악은 Windows에 기본으로 있는 MCI로 재생합니다(mp3, wav 지원). windows.h 매크로가 다른 코드와 부딪히지 않도록 함수 하나만 선언합니다.
struct HWND__;
extern "C" __declspec(dllimport) unsigned long __stdcall mciSendStringW(const wchar_t* Command, wchar_t* ReturnString, unsigned int ReturnLength, HWND__* Callback);
#pragma comment(lib, "winmm.lib")

using namespace Engine;
using namespace Core::Literals;

namespace {

    bool SendMusicCommand(const std::wstring& Command) {
        return mciSendStringW(Command.c_str(), nullptr, 0, nullptr) == 0;
    }

    void SetMusicVolume(float Volume) {
        // MCI 음량은 0~1000입니다.
        SendMusicCommand(L"setaudio macaw_bgm volume to " + std::to_wstring(static_cast<int>(std::clamp(Volume, 0.0f, 1.0f) * 1000.0f)));
    }

    Engine::UActorComponent* FindComponent(const Engine::AActor& Actor, const CoreUObject::UClass* Class) {
        for (const auto& Component : Actor.GetComponents()) {
            if (Component->IsA(Class)) {
                return Component.get();
            }
        }
        return nullptr;
    }

    Engine::UProjectileMovementComponent& ResolveMovement(const Engine::AActor& Actor) {
        if (auto* Movement{static_cast<Engine::UProjectileMovementComponent*>(FindComponent(Actor, Engine::UProjectileMovementComponent::StaticClass()))}) {
            return *Movement;
        }
        throw sol::error{"Actor needs a Projectile Movement component for Velocity"};
    }

    physx::PxShape* GetSinglePhysicsShape(const Engine::UPrimitiveComponent* Component) {
        const auto* Body{Component != nullptr ? Component->GetBodyInstance().GetPxRigidActor() : nullptr};
        physx::PxShape* Shape{};
        return Body != nullptr && Body->getNbShapes() == 1 && Body->getShapes(&Shape, 1) == 1 ? Shape : nullptr;
    }

}

void Engine::StopLuaMusic() {
    SendMusicCommand(L"close macaw_bgm");
}

void Engine::StopLuaMusic() {
    SendMusicCommand(L"close macaw_bgm");
}

AActor& FLuaRuntime::FActorRef::Resolve() const {
    auto* Actor{mActor.Get()};
    if (Actor == nullptr || Actor->IsPendingDestroy()) {
        throw sol::error{"Actor is no longer valid"};
    }
    return *Actor;
}

FLuaRuntime::FLuaComponentRef::FLuaComponentRef(UActorComponent* Component)
    : mComponent(CoreUObject::MakeObjectRef(Component)) {
}

UActorComponent* FLuaRuntime::FLuaComponentRef::Get() const {
    auto* Component{mComponent.Get()};
    const auto* Owner{Component != nullptr ? Component->GetOwner() : nullptr};
    return Owner != nullptr && !Owner->IsPendingDestroy() ? Component : nullptr;
}

UActorComponent& FLuaRuntime::FLuaComponentRef::Resolve() const {
    auto* Component{Get()};
    if (Component == nullptr) {
        throw sol::error{"Component is no longer valid"};
    }
    return *Component;
}

void FLuaRuntime::RegisterBindings(sol::state_view Lua) {
    auto Vector{Lua.new_usertype<FVector>("Vector", sol::constructors<FVector(), FVector(float, float, float)>())};
    Vector["x"] = &FVector::x;
    Vector["y"] = &FVector::y;
    Vector["z"] = &FVector::z;
    Vector["Length"] = &FVector::Length;
    Vector["LengthSquared"] = &FVector::LengthSquared;
    Vector["Normalized"] = &FVector::Normalized;
    Vector[sol::meta_function::addition] = [](const FVector& Left, const FVector& Right) {
        return Left + Right;
    };
    Vector[sol::meta_function::subtraction] = [](const FVector& Left, const FVector& Right) {
        return Left - Right;
    };
    Vector[sol::meta_function::multiplication] = sol::overload([](const FVector& Value, float Scalar) {
        return Value * Scalar;
    }, [](float Scalar, const FVector& Value) {
        return Value * Scalar;
    });

    auto Rotator{Lua.new_usertype<FRotator>("Rotator", sol::constructors<FRotator(), FRotator(float, float, float)>())};
    Rotator["Pitch"] = &FRotator::pitch;
    Rotator["Yaw"] = &FRotator::yaw;
    Rotator["Roll"] = &FRotator::roll;

    auto Quat{Lua.new_usertype<FQuat>("Quat", sol::constructors<FQuat(), FQuat(const FVector&, float)>())};
    Quat["Normalized"] = &FQuat::Normalized;
    Quat["RotateVector"] = &FQuat::RotateVector;
    Quat[sol::meta_function::multiplication] = [](const FQuat& Left, const FQuat& Right) {
        return Left * Right;
    };

    auto Transform{Lua.new_usertype<FTransform>("Transform", sol::constructors<FTransform()>())};
    Transform["Location"] = &FTransform::mLocation;
    Transform["Rotation"] = &FTransform::mRotation;
    Transform["Scale"] = &FTransform::mScale;
    Transform["GetRotator"] = &FTransform::GetRotator;
    Transform["SetRotator"] = &FTransform::SetRotator;

    Lua.new_enum<Engine::EDepthOfFieldApertureShape>("DepthOfFieldApertureShape", {
        {"Circle", Engine::EDepthOfFieldApertureShape::Circle},
        {"Pentagon", Engine::EDepthOfFieldApertureShape::Pentagon},
        {"Hexagon", Engine::EDepthOfFieldApertureShape::Hexagon}
    });

    // 액터·컴포넌트에 지정할 수 있는 그룹만 노출합니다. 내부 경계 그룹은 SetTickGroup이 거부합니다.
    Lua.new_enum<Engine::ETickingGroup>("TickGroup", {
        {"PrePhysics", Engine::ETickingGroup::PrePhysics},
        {"DuringPhysics", Engine::ETickingGroup::DuringPhysics},
        {"PostPhysics", Engine::ETickingGroup::PostPhysics},
        {"PostUpdateWork", Engine::ETickingGroup::PostUpdateWork}
    });

    auto DepthOfField{Lua.new_usertype<Engine::FDepthOfFieldSettings>("DepthOfFieldSettings", sol::constructors<Engine::FDepthOfFieldSettings()>())};
    DepthOfField["Enabled"] = &Engine::FDepthOfFieldSettings::mEnabled;
    DepthOfField["FocalDistance"] = sol::property([](const Engine::FDepthOfFieldSettings& Settings) {
        return Settings.mFocalDistance.GetBase();
    }, [](Engine::FDepthOfFieldSettings& Settings, float Meters) {
        Settings.mFocalDistance = Core::FLength::FromBase(Meters);
    });
    DepthOfField["MaxBlurRadius"] = &Engine::FDepthOfFieldSettings::mMaxBlurRadius;
    DepthOfField["FStop"] = &Engine::FDepthOfFieldSettings::mFStop;
    DepthOfField["ApertureShape"] = &Engine::FDepthOfFieldSettings::mApertureShape;
    DepthOfField["NearBlur"] = &Engine::FDepthOfFieldSettings::mNearBlur;
    DepthOfField["FarBlur"] = &Engine::FDepthOfFieldSettings::mFarBlur;

    auto Hit{Lua.new_usertype<Engine::FHitResult>("HitResult", sol::no_constructor)};
    Hit["BlockingHit"] = sol::readonly(&Engine::FHitResult::mBlockingHit);
    Hit["ImpactPoint"] = sol::readonly(&Engine::FHitResult::mImpactPoint);
    Hit["Normal"] = sol::readonly(&Engine::FHitResult::mNormal);
    Hit["Time"] = sol::readonly(&Engine::FHitResult::mTime);
    Hit["Distance"] = sol::property([](const Engine::FHitResult& Value) {
        return Value.mDistance.GetBase();
    });
    Hit["Location"] = sol::readonly(&Engine::FHitResult::mLocation);
    Hit["ImpactNormal"] = sol::readonly(&Engine::FHitResult::mImpactNormal);
    Hit["StartPenetrating"] = sol::readonly(&Engine::FHitResult::mStartPenetrating);
    Hit["PenetrationDepth"] = sol::property([](const Engine::FHitResult& Value) {
        return Value.mPenetrationDepth.GetBase();
    });
    Hit["SurfaceFriction"] = sol::property([](const Engine::FHitResult& Value) -> sol::optional<float> {
        const auto* Shape{GetSinglePhysicsShape(Value.GetComponent())};
        physx::PxMaterial* Material{};
        if (Shape != nullptr && Shape->getMaterials(&Material, 1) == 1 && Material != nullptr) {
            return Material->getDynamicFriction();
        }
        return sol::nullopt;
    });

    auto RaycastHit{Lua.new_usertype<Engine::FWorldHit>("RaycastHit", sol::no_constructor)};
    RaycastHit["IsHit"] = sol::property([](const Engine::FWorldHit& Value) {
        return Value.mActor.Get() != nullptr;
    });
    RaycastHit["Position"] = sol::readonly(&Engine::FWorldHit::mPosition);
    RaycastHit["Distance"] = sol::property([](const Engine::FWorldHit& Value) {
        return Value.mDistance.GetBase();
    });

    auto Component{Lua.new_usertype<FLuaComponentRef>("Component", sol::no_constructor)};
    Component["IsValid"] = [](const FLuaComponentRef& Ref) {
        return Ref.Get() != nullptr;
    };
    Component["Name"] = sol::property([](const FLuaComponentRef& Ref) {
        return Ref.Resolve().GetName();
    });
    Component["Owner"] = sol::property([](const FLuaComponentRef& Ref) {
        return FActorRef{CoreUObject::MakeObjectRef(Ref.Resolve().GetOwner())};
    });
    Component["GetTickGroup"] = [](const FLuaComponentRef& Ref) {
        return Ref.Resolve().GetTickGroup();
    };
    // 변경은 다음 프레임 배치부터 적용됩니다.
    Component["SetTickGroup"] = [](const FLuaComponentRef& Ref, Engine::ETickingGroup Group) {
        return Ref.Resolve().SetTickGroup(Group);
    };
    Component["SetAnimGraphFloat"] =[](const FLuaComponentRef& Ref, const Core::FString& Name, float Value) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetAnimGraphFloat(Name, Value);
    };
    Component["SetAnimGraphBool"] = [](const FLuaComponentRef& Ref, const Core::FString& Name, bool Value) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetAnimGraphBool(Name, Value);
    };
    Component["RestartAnimGraphPlayer"] = [](const FLuaComponentRef& Ref, const Core::FString& Name) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->RestartAnimGraphPlayer(Name);
    };
    Component["IsAnimGraphPlayerFinished"] = [](const FLuaComponentRef& Ref, const Core::FString& Name) {
        const auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->IsAnimGraphPlayerFinished(Name);
    };
    Component["GetDepthOfFieldSettings"] = [](const FLuaComponentRef& Ref) -> sol::optional<Engine::FDepthOfFieldSettings> {
        if (const auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())}) {
            return Camera->GetDepthOfFieldSettings();
        }

        return sol::nullopt;
    };
    Component["SetDepthOfFieldSettings"] = [](const FLuaComponentRef& Ref, const Engine::FDepthOfFieldSettings& Settings) {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())};
        return Camera != nullptr && Camera->SetDepthOfFieldSettings(Settings);
    };
    Component["SetDepthOfFieldFocalDistance"] = [](const FLuaComponentRef& Ref, float Distance) {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())};
        return Camera != nullptr && Camera->SetDepthOfFieldFocalDistance(Core::FLength::FromBase(Distance));
    };
    // 렌즈 값은 mm 단위로 주고받습니다. 카메라 컴포넌트가 아니면 nil 또는 false를 반환합니다.
    Component["GetFocalLength"] = [](const FLuaComponentRef& Ref) -> sol::optional<float> {
        if (const auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())}) {
            return Camera->GetFocalLength().GetBase() * 1000.0f;
        }

        return sol::nullopt;
    };
    Component["SetFocalLength"] = [](const FLuaComponentRef& Ref, float Millimeters) {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())};
        return Camera != nullptr && Camera->SetFocalLength(Core::FLength::FromBase(Millimeters * 0.001f));
    };
    Component["GetSensorHeight"] = [](const FLuaComponentRef& Ref) -> sol::optional<float> {
        if (const auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())}) {
            return Camera->GetSensorHeight().GetBase() * 1000.0f;
        }

        return sol::nullopt;
    };
    Component["SetSensorHeight"] = [](const FLuaComponentRef& Ref, float Millimeters) {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())};
        return Camera != nullptr && Camera->SetSensorHeight(Core::FLength::FromBase(Millimeters * 0.001f));
    };
    Component["SetUseFieldOfView"] = [](const FLuaComponentRef& Ref, bool UseFieldOfView) {
        auto* Camera{CoreUObject::Cast<Engine::UCameraComponent>(&Ref.Resolve())};
        return Camera != nullptr && Camera->SetUseFieldOfView(UseFieldOfView);
    };
    Component["SetDriveTorque"] = [](const FLuaComponentRef& Ref, float Torque, Uint32 WheelIndex) {
        auto* Movement{CoreUObject::Cast<Engine::USimpleVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr && Movement->SetDriveTorque(Torque * 1_Nm, WheelIndex);
    };
    Component["SetBrakeTorque"] = [](const FLuaComponentRef& Ref, float Torque, Uint32 WheelIndex) {
        auto* Movement{CoreUObject::Cast<Engine::USimpleVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr && Movement->SetBrakeTorque(Torque * 1_Nm, WheelIndex);
    };
    Component["SetSteerAngle"] = [](const FLuaComponentRef& Ref, float AngleDegrees, Uint32 WheelIndex) {
        auto* Movement{CoreUObject::Cast<Engine::USimpleVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr && Movement->SetSteerAngle(AngleDegrees * 1_deg, WheelIndex);
    };
    Component["SetDriveInput"] = [](const FLuaComponentRef& Ref, float Throttle, float Brake, float Steering) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr && Movement->SetDriveInput(Throttle, Brake, Steering);
    };
    Component["ResetDrive"] = [](const FLuaComponentRef& Ref) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr && Movement->ResetDrive();
    };
    Component["SetWheelMesh"] = [](const FLuaComponentRef& Ref, Uint32 WheelIndex, sol::optional<FLuaComponentRef> MeshRef) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        auto* Mesh{MeshRef ? CoreUObject::Cast<Engine::UStaticMeshComponent>(&MeshRef->Resolve()) : nullptr};

        // nil은 연결 해제이고, 스태틱 메시가 아닌 컴포넌트는 거부합니다.
        return Movement != nullptr && (!MeshRef || Mesh != nullptr) && Movement->SetWheelMesh(WheelIndex, Mesh);
    };
    Component["SetWheelSkeletalMesh"] = [](const FLuaComponentRef& Ref, sol::optional<FLuaComponentRef> MeshRef) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        auto* Mesh{MeshRef ? CoreUObject::Cast<Engine::USkeletalMeshComponent>(&MeshRef->Resolve()) : nullptr};

        return Movement != nullptr && (!MeshRef || Mesh != nullptr) && Movement->SetWheelSkeletalMesh(Mesh);
    };
    Component["SetWheelBone"] = [](const FLuaComponentRef& Ref, Uint32 WheelIndex, sol::optional<Core::FString> BoneName) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};

        return Movement != nullptr && Movement->SetWheelBone(WheelIndex, BoneName.value_or(Core::FString{}));
    };
    // 차종 교체용입니다. 바퀴 수와 서스펜션 값은 유지하고 위치·반지름·폭만 바꾸며, 값이 바뀌면 차량을 다시 등록합니다.
    Component["SetWheelGeometry"] = [](const FLuaComponentRef& Ref, const sol::table& Wheels) {
        auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        if (Movement == nullptr || Wheels.size() != Movement->GetWheelSettings().size()) {
            return false;
        }

        auto Settings{Movement->GetWheelSettings()};
        for (Index_t Index{}; Index < Settings.size(); ++Index) {
            const auto Wheel{Wheels.get<sol::optional<sol::table>>(Index + 1)};
            if (!Wheel) {
                return false;
            }

            const auto Offset{Wheel->get<sol::optional<FVector>>("Offset")};
            const auto Radius{Wheel->get<sol::optional<float>>("Radius")};
            const auto Width{Wheel->get<sol::optional<float>>("Width")};
            if (!Offset || !Radius || !Width || !Offset->IsFinite()) {
                return false;
            }

            Settings[Index].mCenterOffset = *Offset;
            Settings[Index].mRadius = Core::FLength::FromBase(*Radius);
            Settings[Index].mWidth = Core::FLength::FromBase(*Width);
        }

        return Movement->SetWheelSettings(Settings);
    };
    // 스켈레톤을 먼저 바꿔 새 메시가 이전 스켈레톤과 짝지어진 포즈를 거치지 않게 합니다.
    Component["SetSkeletalMesh"] = [](const FLuaComponentRef& Ref, const Core::FString& MeshPath, const Core::FString& SkeletonPath) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};

        return Mesh != nullptr && Mesh->SetSkeletonPath(SkeletonPath) && Mesh->SetMeshPath(MeshPath) && Mesh->GetSkeletalMesh() != nullptr && Mesh->GetSkeleton() != nullptr;
    };
    Component["SetSkeletalCollisionEnabled"] = [](const FLuaComponentRef& Ref, bool Enabled) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        if (Mesh == nullptr) {
            return false;
        }

        Mesh->SetCollisionEnabled(Enabled);
        return true;
    };
    Component["GetBoneWorldLocation"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName) -> sol::optional<FVector> {
        const auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        FTransform Transform{};
        if (Mesh != nullptr && Mesh->GetBoneWorldTransform(Mesh->GetBoneIndex(BoneName), Transform)) {
            return Transform.mLocation;
        }
        return sol::nullopt;
    };
    Component["GetDriveState"] = [](const FLuaComponentRef& Ref) {
        const auto* Movement{CoreUObject::Cast<Engine::UVehicleMovementComponent>(&Ref.Resolve())};
        return Movement != nullptr ? std::tuple{Movement->GetCurrentGear(), Movement->GetForwardSpeed().GetBase()} : std::tuple{Int32{}, 0.0f};
    };
    Component["SetText"] = [](const FLuaComponentRef& Ref, const Core::FString& Text) {
        auto* Billboard{CoreUObject::Cast<Engine::UBillboardTextComponent>(&Ref.Resolve())};
        return Billboard != nullptr && Billboard->SetText(Text);
    };
    Component["SetVehicleDrivable"] = [](const FLuaComponentRef& Ref, bool Drivable) {
        const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(&Ref.Resolve())};
        auto* Body{Primitive != nullptr ? Primitive->GetBodyInstance().GetPxRigidActor() : nullptr};
        const auto Count{Body != nullptr ? Body->getNbShapes() : 0u};

        for (physx::PxU32 Index{}; Index < Count; ++Index) {
            physx::PxShape* Shape{};
            if (Body->getShapes(&Shape, 1, Index) == 1) {
                PhysicsRuntime::FVehicleManager::SetDrivableSurface(*Shape, Drivable);
            }
        }

        return Count != 0;
    };
    Component["SetPhysicsVelocity"] = [](const FLuaComponentRef& Ref, const FVector& Linear, const FVector& Angular) {
        auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(&Ref.Resolve())};
        return Primitive != nullptr && Linear.IsFinite() && Angular.IsFinite() &&
               Primitive->SetPhysicsLinearVelocity(Linear) && Primitive->SetPhysicsAngularVelocity(Angular);
    };
    Component["SetPhysicsFriction"] = [](const FLuaComponentRef& Ref, float StaticFriction, float DynamicFriction) {
        auto& Target{Ref.Resolve()};
        auto* Shape{GetSinglePhysicsShape(CoreUObject::Cast<Engine::UPrimitiveComponent>(&Target))};
        auto* World{Target.GetWorld()};
        auto* Runtime{World != nullptr ? World->GetPhysicsRuntime() : nullptr};
        auto* Physics{Runtime != nullptr ? Runtime->GetPhysics() : nullptr};
        if (Shape == nullptr || Physics == nullptr || !std::isfinite(StaticFriction) || !std::isfinite(DynamicFriction) || DynamicFriction < 0.0f || StaticFriction < DynamicFriction) {
            return false;
        }

        auto* Material{Physics->createMaterial(StaticFriction, DynamicFriction, 0.0f)};
        if (Material == nullptr) {
            return false;
        }

        Material->setFrictionCombineMode(physx::PxCombineMode::eMIN);
        Shape->setMaterials(&Material, 1);
        Material->release();
        return true;
    };
    Component["AddForce"] = [](const FLuaComponentRef& Ref, const FVector& Force) {
        auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(&Ref.Resolve())};
        return Primitive != nullptr && Primitive->IsSimulatingPhysics() && Force.IsFinite() && Primitive->AddForce(Force);
    };
    Component["SetSimulatePhysics"] = [](const FLuaComponentRef& Ref, bool Simulate) {
        auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(&Ref.Resolve())};
        if (Primitive == nullptr) {
            return false;
        }

        Primitive->SetSimulatePhysics(Simulate);
        return true;
    };
    Component["SetBodySimulatePhysics"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, bool Simulate) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetBodySimulatePhysics(BoneName, Simulate);
    };
    Component["SetAllBodiesBelowSimulatePhysics"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, bool Simulate, sol::optional<bool> IncludeSelf) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, Simulate, IncludeSelf.value_or(true));
    };
    Component["IsBodySimulatingPhysics"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName) {
        const auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->IsBodySimulatingPhysics(BoneName);
    };
    Component["SetBodyPhysicsBlendWeight"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, float Weight) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetBodyPhysicsBlendWeight(BoneName, Weight);
    };
    Component["SetAllBodiesBelowPhysicsBlendWeight"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, float Weight, sol::optional<bool> IncludeSelf) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, Weight, IncludeSelf.value_or(true));
    };
    Component["AddImpulseToBody"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, const FVector& Impulse, sol::optional<bool> VelocityChange) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->AddImpulseToBody(BoneName, Impulse, VelocityChange.value_or(false));
    };
    Component["AddImpulseAtLocationToBody"] = [](const FLuaComponentRef& Ref, const Core::FString& BoneName, const FVector& Impulse, const FVector& Location) {
        auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Ref.Resolve())};
        return Mesh != nullptr && Mesh->AddImpulseAtLocationToBody(BoneName, Impulse, Location);
    };
    Component["SetHiddenInGame"] = [](const FLuaComponentRef& Ref, bool Hidden) {
        auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(&Ref.Resolve())};
        if (Primitive == nullptr) {
            return false;
        }

        Primitive->SetHiddenInGame(Hidden);
        return true;
    };
    // 이 컴포넌트의 스케일만 바꿉니다. Actor:SetActorTransform과 달리 다른 컴포넌트에 비율을 전파하지 않으며,
    // 물리 바디가 있으면 새 크기로 다시 만듭니다(차량이면 바디 재생성 이벤트로 다시 등록됩니다).
    Component["SetComponentScale"] = [](const FLuaComponentRef& Ref, const FVector& Scale) {
        auto* Scene{CoreUObject::Cast<Engine::USceneComponent>(&Ref.Resolve())};
        if (Scene == nullptr || !Scale.IsFinite() || Scale.x <= 0.0f || Scale.y <= 0.0f || Scale.z <= 0.0f) {
            return false;
        }

        FTransform Transform{Scene->GetTransform()};
        Transform.mScale = Scale;
        return Scene->SetTransform(Transform);
    };
    Lua["SkeletalMeshComponent"] = Lua.get<sol::table>("Component");
    Lua["CameraComponent"] = Lua.get<sol::table>("Component");

    auto Actor{Lua.new_usertype<FActorRef>("Actor", sol::no_constructor)};
    Actor["IsValid"] = [](const FActorRef& Ref) {
        const auto* Target{Ref.mActor.Get()};
        return Target != nullptr && !Target->IsPendingDestroy();
    };
    Actor["GetName"] = [](const FActorRef& Ref) {
        return Ref.Resolve().GetName();
    };
    Actor["UUID"] = sol::property([](const FActorRef& Ref) {
        return Ref.mActor.GetGuid().ToString();
    });
    Actor["GetTickGroup"] = [](const FActorRef& Ref) {
        return Ref.Resolve().GetTickGroup();
    };
    Actor["SetTickGroup"] = [](const FActorRef& Ref, Engine::ETickingGroup Group) {
        return Ref.Resolve().SetTickGroup(Group);
    };
    Actor["GetActorTransform"] =[](const FActorRef& Ref) {
        return Ref.Resolve().GetActorTransform();
    };
    Actor["SetActorTransform"] = [](const FActorRef& Ref, const FTransform& Value) {
        return Ref.Resolve().SetActorTransform(Value);
    };
    Actor["GetActorLocation"] = [](const FActorRef& Ref) {
        return Ref.Resolve().GetActorLocation();
    };
    Actor["SetActorLocation"] = [](const FActorRef& Ref, const FVector& NewLocation, sol::optional<bool> Sweep) {
        if (!NewLocation.IsFinite()) {
            throw sol::error{"Invalid actor location"};
        }

        Engine::FHitResult Hit{};
        const bool Moved{Ref.Resolve().SetActorLocation(NewLocation, Sweep.value_or(false), &Hit)};

        return std::make_tuple(Moved, Hit);
    };
    Actor["AddActorWorldOffset"] = [](const FActorRef& Ref, const FVector& DeltaLocation, sol::optional<bool> Sweep) {
        if (!DeltaLocation.IsFinite()) {
            throw sol::error{"Invalid actor offset"};
        }

        Engine::FHitResult Hit{};
        const bool Moved{Ref.Resolve().AddActorWorldOffset(DeltaLocation, Sweep.value_or(false), &Hit)};

        return std::make_tuple(Moved, Hit);
    };
    Actor["GetComponentByClass"] = [](const FActorRef& Ref, const Core::FString& ClassName) -> sol::optional<FLuaComponentRef> {
        const auto* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};
        if (Class == nullptr || !Class->IsChildOf(Engine::UActorComponent::StaticClass())) {
            throw sol::error{"Invalid component class: " + ClassName};
        }
        if (auto* Found{FindComponent(Ref.Resolve(), Class)}) {
            return FLuaComponentRef{Found};
        }
        return sol::nullopt;
    };
    Actor["GetComponentByName"] = [](const FActorRef& Ref, const Core::FString& Name) -> sol::optional<FLuaComponentRef> {
        for (const auto& Component : Ref.Resolve().GetComponents()) {
            if (Component->GetName() == Name) {
                return FLuaComponentRef{Component.get()};
            }
        }
        return sol::nullopt;
    };
    Actor["GetSkeletalMeshComponent"] =[](const FActorRef& Ref) -> sol::optional<FLuaComponentRef> {
        if (auto* Found{FindComponent(Ref.Resolve(), Engine::USkeletalMeshComponent::StaticClass())}) {
            return FLuaComponentRef{Found};
        }
        return sol::nullopt;
    };
    Actor["GetCameraComponent"] = [](const FActorRef& Ref) -> sol::optional<FLuaComponentRef> {
        if (auto* Found{FindComponent(Ref.Resolve(), Engine::UCameraComponent::StaticClass())}) {
            return FLuaComponentRef{Found};
        }
        return sol::nullopt;
    };
    Actor["Location"] = sol::property([](const FActorRef& Ref) {
        return Ref.Resolve().GetActorTransform().mLocation;
    }, [](const FActorRef& Ref, const FVector& Value) {
        auto& Target{Ref.Resolve()};
        auto Pose{Target.GetActorTransform()};
        Pose.mLocation = Value;
        if (!Target.SetActorTransform(Pose)) {
            throw sol::error{"Invalid actor transform"};
        }
    });
    Actor["Velocity"] = sol::property([](const FActorRef& Ref) {
        return ResolveMovement(Ref.Resolve()).GetVelocity();
    }, [](const FActorRef& Ref, const FVector& Value) {
        if (!ResolveMovement(Ref.Resolve()).SetVelocity(Value)) {
            throw sol::error{"Invalid movement velocity"};
        }
    });
    Actor["StopMovement"] = [](const FActorRef& Ref) {
        ResolveMovement(Ref.Resolve()).StopMovement();
    };
    Actor["PrintLocation"] = [](const FActorRef& Ref) {
        const auto& Target{Ref.Resolve()};
        const auto Location{Target.GetActorTransform().mLocation};
        std::clog << Target.GetName() << " Location " << Location.x << ' ' << Location.y << ' ' << Location.z << '\n';
    };

    auto World{Lua.create_named_table("World")};
    World["FindActor"] = [this](const Core::FString& Name) -> sol::optional<FActorRef> {
        if (auto* Found{mWorld.FindActor(Name)}) {
            return FActorRef{CoreUObject::MakeObjectRef(Found)};
        }
        return sol::nullopt;
    };
    World["Raycast"] = [this](const FVector& Origin, const FVector& Direction, const sol::optional<FActorRef>& Ignored) {
        return mWorld.Raycast(Origin, Direction, Ignored ? &Ignored->Resolve() : nullptr);
    };
    World["SetActiveCamera"] = [this](const FActorRef& Ref) {
        auto* Camera{static_cast<Engine::UCameraComponent*>(FindComponent(Ref.Resolve(), Engine::UCameraComponent::StaticClass()))};
        return Camera != nullptr && mWorld.SetActiveCamera(Camera);
    };

    Lua.new_enum<Core::EKey>("Key", {{"W", Core::EKey::W}, {"A", Core::EKey::A}, {"S", Core::EKey::S}, {"D", Core::EKey::D}, {"C", Core::EKey::C}, {"V", Core::EKey::V}, {"F", Core::EKey::F}, {"Q", Core::EKey::Q}, {"R", Core::EKey::R}, {"E", Core::EKey::E}, {"X", Core::EKey::X}, {"K", Core::EKey::K}, {"H", Core::EKey::H}, {"Z", Core::EKey::Z}, {"Space", Core::EKey::Space}, {"Backspace", Core::EKey::Backspace}, {"Delete", Core::EKey::Delete}, {"Shift", Core::EKey::Shift}, {"LeftShift", Core::EKey::LeftShift}, {"RightShift", Core::EKey::RightShift}, {"MouseLeft", Core::EKey::MouseLeft}, {"MouseRight", Core::EKey::MouseRight}});

    // 배경 음악 하나를 틀고 끕니다. 경로는 스크립트처럼 Content 기준입니다. 예: Audio.PlayMusic("Audio/BGM.mp3", 0.8, true, 5) — 5초 지점부터
    auto Audio{Lua.create_named_table("Audio")};
    Audio["PlayMusic"] = [this](const Core::FString& Path, sol::optional<float> Volume, sol::optional<bool> Loop, sol::optional<float> StartSeconds) {
        Core::FString Error{};
        const auto* Assets{mWorld.GetAssets()};
        const auto Resolved{Assets != nullptr ? Assets->ResolvePath(Core::FilePath::FromUtf8(Path), Error) : std::filesystem::path{}};
        std::error_code Missing{};

        if (Resolved.empty() || !std::filesystem::is_regular_file(Resolved, Missing)) {
            throw sol::error{"Cannot find music file: " + Path};
        }

        StopLuaMusic();

        if (!SendMusicCommand(L"open \"" + Resolved.wstring() + L"\" type mpegvideo alias macaw_bgm")) {
            return false;
        }

        SetMusicVolume(Volume.value_or(1.0f));

        // 시작 위치(초)부터 재생합니다. 반복하면 곡 처음부터 다시 돕니다.
        SendMusicCommand(L"set macaw_bgm time format milliseconds");
        const auto From{std::to_wstring(static_cast<long long>(std::max(StartSeconds.value_or(0.0f), 0.0f) * 1000.0f))};

        return SendMusicCommand(L"play macaw_bgm from " + From + (Loop.value_or(true) ? L" repeat" : L""));
    };
    Audio["SetMusicVolume"] = [](float Volume) {
        SetMusicVolume(Volume);
    };
    Audio["StopMusic"] = [] {
        StopLuaMusic();
    };

    auto Input{Lua.create_named_table("Input")};
    Input["IsDown"] = [this](Core::EKey Key) {
        return mFrameInput.IsDown(Key, Core::EInputRoute::Game);
    };
    Input["WasPressed"] = [this](Core::EKey Key) {
        return mFrameInput.WasPressed(Key, Core::EInputRoute::Game);
    };
    Input["GetMouseDelta"] = [this] {
        const auto Delta{mFrameInput.GetMouseDelta(Core::EInputRoute::Game)};
        return std::tuple{Delta.x, Delta.y};
    };
}
