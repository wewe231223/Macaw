#include "FLuaRuntime.h"

#include "Core/Math/FRotator.h"
#include "Core/Math/FQuat.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/FReflectionRegistry.h"
#include "Engine/AActor.h"
#include "Engine/Component/UActorComponent.h"
#include "Engine/Component/UCameraComponent.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/FWorldHit.h"
#include "Engine/UWorld.h"

#include <iostream>
#include <tuple>

using namespace Engine;

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

    auto Hit{Lua.new_usertype<Engine::FHitResult>("HitResult", sol::no_constructor)};
    Hit["BlockingHit"] = sol::readonly(&Engine::FHitResult::mBlockingHit);
    Hit["ImpactPoint"] = sol::readonly(&Engine::FHitResult::mImpactPoint);
    Hit["Normal"] = sol::readonly(&Engine::FHitResult::mNormal);
    Hit["NormalImpulse"] = sol::readonly(&Engine::FHitResult::mNormalImpulse);
    Hit["Time"] = sol::readonly(&Engine::FHitResult::mTime);

    auto RaycastHit{Lua.new_usertype<Engine::FWorldHit>("RaycastHit", sol::no_constructor)};
    RaycastHit["IsHit"] = sol::property([](const Engine::FWorldHit& Value) {
        return Value.mActor.Get() != nullptr;
    });
    RaycastHit["Position"] = sol::readonly(&Engine::FWorldHit::mPosition);
    RaycastHit["Distance"] = sol::readonly(&Engine::FWorldHit::mDistance);

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
    Component["SetAnimGraphFloat"] = [](const FLuaComponentRef& Ref, const Core::FString& Name, float Value) {
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
    Lua["SkeletalMeshComponent"] = Lua.get<sol::table>("Component");

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
    Actor["GetActorTransform"] = [](const FActorRef& Ref) {
        return Ref.Resolve().GetActorTransform();
    };
    Actor["SetActorTransform"] = [](const FActorRef& Ref, const FTransform& Value) {
        return Ref.Resolve().SetActorTransform(Value);
    };
    Actor["GetComponentByClass"] = [](const FActorRef& Ref, const Core::FString& ClassName) -> sol::optional<FLuaComponentRef> {
        const auto* Class{CoreUObject::FReflectionRegistry::FindClass(ClassName)};
        if (Class == nullptr || !Class->IsChildOf(Engine::UActorComponent::StaticClass())) {
            throw sol::error{"Invalid component class: " + ClassName};
        }
        if (auto* Found{Ref.Resolve().FindComponentByClass(Class)}) {
            return FLuaComponentRef{Found};
        }
        return sol::nullopt;
    };
    Actor["GetSkeletalMeshComponent"] = [](const FActorRef& Ref) -> sol::optional<FLuaComponentRef> {
        if (auto* Found{Ref.Resolve().FindComponentByClass(Engine::USkeletalMeshComponent::StaticClass())}) {
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
        auto* Camera{static_cast<Engine::UCameraComponent*>(Ref.Resolve().FindComponentByClass(Engine::UCameraComponent::StaticClass()))};
        return Camera != nullptr && mWorld.SetActiveCamera(Camera);
    };

    Lua.new_enum<Core::EKey>("Key", {{"W", Core::EKey::W}, {"A", Core::EKey::A}, {"S", Core::EKey::S}, {"D", Core::EKey::D}, {"C", Core::EKey::C}, {"V", Core::EKey::V}, {"F", Core::EKey::F}, {"Q", Core::EKey::Q}, {"R", Core::EKey::R}, {"E", Core::EKey::E}, {"X", Core::EKey::X}, {"K", Core::EKey::K}, {"H", Core::EKey::H}, {"Z", Core::EKey::Z}, {"Space", Core::EKey::Space}, {"Backspace", Core::EKey::Backspace}, {"Delete", Core::EKey::Delete}, {"Shift", Core::EKey::Shift}, {"LeftShift", Core::EKey::LeftShift}, {"RightShift", Core::EKey::RightShift}, {"MouseLeft", Core::EKey::MouseLeft}, {"MouseRight", Core::EKey::MouseRight}});

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
