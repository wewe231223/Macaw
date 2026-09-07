#include "PCH.h"
#include "UWorld.h"

#include <algorithm>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"
#include "Component/UCollisionComponent.h"
#include "FMouseCameraRotateRequestMessage.h"
#include "FMousePickRequestMessage.h"
#include "FWorldSelectionChangedMessage.h"
#include "FKeyboardCameraMoveRequestMessage.h"

UWorld::~UWorld()
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        UObjectSystem::Unregister(Actor.get(), Actor->GetHandle());
    }

    Actors.clear();
}

const std::vector<std::unique_ptr<AActor>>& UWorld::GetActors() const
{
    return Actors;
}

FRenderProbe& UWorld::BuildRenderProbe() 
{
	Probe.ActorProbes.clear();

    for (const UStaticMeshComponent* Component : RenderableComponents)
    {
        Component->MakeRender(Probe);
    }

    if (Camera != nullptr)
    {
        Probe.MainCameraProbe.View =
            Camera->GetViewMatrix();

        Probe.MainCameraProbe.Projection =
            Camera->GetProjectionMatrix();

        Probe.MainCameraProbe.ViewProjection =
            Camera->GetViewProjectionMatrix();
    }
    return Probe;
}

void UWorld::Tick(float DeltaTime)
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        Actor->Tick(DeltaTime);
    }
}

void UWorld::RegisterRenderable(UStaticMeshComponent* Component)
{
    if (Component == nullptr || std::ranges::find(RenderableComponents, Component) != RenderableComponents.end())
    {
        return;
    }

    RenderableComponents.push_back(Component);
}

void UWorld::UnregisterRenderable(UStaticMeshComponent* Component)
{
    std::erase(RenderableComponents, Component);
}

void UWorld::SetMainCamera(UCameraComponent* InCamera)
{
    Camera = InCamera;
}

void UWorld::ClearMainCamera(UCameraComponent* InCamera)
{
    if (Camera == InCamera)
    {
        Camera = nullptr;
    }
}

void UWorld::SetAssetRegistry(FAssetRegistry* InAssetRegistry)
{
    AssetRegistry = InAssetRegistry;
}

FAssetRegistry* UWorld::GetAssetRegistry() const
{
    return AssetRegistry;
}

void UWorld::InitializeEditorEventSender(
    FMessageChannel::FSender&& InSender)
{
    EditorEventSender.emplace(std::move(InSender));
}

void UWorld::HandleMousePickRequest(
    const FMousePickRequestMessage& Message)
{
    FObjectHandle SelectedComponentHandle{};

    if (Camera != nullptr &&
        Message.ViewportWidth != 0 &&
        Message.ViewportHeight != 0)
    {
        const float NdcX =
            (2.0f * static_cast<float>(Message.ScreenX) /
                static_cast<float>(Message.ViewportWidth)) -
            1.0f;

        const float NdcY =
            1.0f -
            (2.0f * static_cast<float>(Message.ScreenY) /
                static_cast<float>(Message.ViewportHeight));

        const FMatrix InverseViewProjection =
            Camera->GetViewProjectionMatrix().Invert();

        const FVector3 RayOrigin = FVector3::Transform(
            FVector3{ NdcX, NdcY, 0.0f },
            InverseViewProjection);

        FVector3 RayDirection = FVector3::Transform(
            FVector3{ NdcX, NdcY, 1.0f },
            InverseViewProjection) - RayOrigin;

        if (RayDirection.LengthSquared() > 0.0f)
        {
            RayDirection.Normalize();

            float NearestDistance = std::numeric_limits<float>::max();
            UCollisionComponent* NearestCollision = nullptr;

            for (const TObjectRef<UCollisionComponent>& CollisionRef : CollisionComponents)
            {
                UCollisionComponent* CollisionComponent = CollisionRef.Get();

                if (CollisionComponent == nullptr)
                {
                    continue;
                }

                float HitDistance = 0.0f;

                if (CollisionComponent->Raycast(
                    FRay{ RayOrigin, RayDirection },
                    HitDistance) &&
                    HitDistance < NearestDistance)
                {
                    NearestDistance = HitDistance;
                    NearestCollision = CollisionComponent;
                }
            }

            if (NearestCollision != nullptr)
            {
                AActor* Owner = NearestCollision->GetOwner();

                if (Owner != nullptr)
                {
                    if (USceneComponent* RootComponent = Owner->GetRootComponent())
                    {
                        SelectedComponentHandle = RootComponent->GetHandle();
                    }
                }
            }
            else
            {
                SelectedComponentHandle = {};
            }
        }
    }

    if (EditorEventSender.has_value())
    {
        EditorEventSender->TryEmplace<FWorldSelectionChangedMessage>(
            SelectedComponentHandle);
    }
}

void UWorld::HandleMouseCameraRotateRequest(const FMouseCameraRotateRequestMessage& Message)
{
    if (Camera == nullptr)
    {
        return;
    }

    constexpr float RotationSensitivity = 0.003f;
    constexpr float MaximumPitch = 1.5f;

    FTransform& CameraTransform = Camera->GetTransform();
    FRotator Rotation = CameraTransform.GetRotation();

    Rotation.y += Message.DeltaX * RotationSensitivity;

    Rotation.x = std::clamp(
        Rotation.x - Message.DeltaY * RotationSensitivity,
        -MaximumPitch,
        MaximumPitch);

    CameraTransform.SetRotation(Rotation);
}

AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) 
{
    if (!InActor)
    {
        return nullptr;
    }

    AActor* Actor = InActor.get();

    // 아직 등록되지 않은 Actor만 등록
    if (UObjectSystem::Resolve(Actor->GetHandle()) != Actor)
    {
        UObjectSystem::Register(Actor);
    }

    // 먼저 World가 소유권을 확보
    Actors.push_back(std::move(InActor));

    // 컴포넌트 OnCreate 호출보다 먼저 World가 소유하고 있어야 함
    Actor->SetWorld(this);

    return Actor;
}

void UWorld::RegisterCollision(UCollisionComponent* Component)
{
    if (Component == nullptr)
    {
        return;
    }

    const auto FoundComponent = std::ranges::find_if(
        CollisionComponents,
        [Component](const TObjectRef<UCollisionComponent>& ComponentRef)
        {
            return ComponentRef.Get() == Component;
        });

    if (FoundComponent != CollisionComponents.end())
    {
        return;
    }

    CollisionComponents.emplace_back(Component);
}

void UWorld::UnregisterCollision(UCollisionComponent* Component)
{
    std::erase_if(
        CollisionComponents,
        [Component](const TObjectRef<UCollisionComponent>& ComponentRef)
        {
            UCollisionComponent* RegisteredComponent = ComponentRef.Get();

            return RegisteredComponent == nullptr || RegisteredComponent == Component;
        });
}

void UWorld::HandleKeyboardCameraMoveRequest(
    const FKeyboardCameraMoveRequestMessage& Message)
{
    if (Camera == nullptr || Message.DeltaTime <= 0.0f)
    {
        return;
    }

    const FMatrix CameraWorldMatrix = Camera->GetWorldMatrix();

    const FVector3 ForwardDirection = CameraWorldMatrix.Backward();

    const FVector3 RightDirection = CameraWorldMatrix.Right();

    FVector3 MoveDirection = ForwardDirection * Message.ForwardAxis + RightDirection * Message.RightAxis;

    if (MoveDirection.LengthSquared() <= 0.0f)
    {
        return;
    }

    MoveDirection.Normalize();

    constexpr float CameraMoveSpeed = 5.0f;

    FTransform& CameraTransform = Camera->GetTransform();

    CameraTransform.SetPosition(CameraTransform.GetPosition() + MoveDirection * CameraMoveSpeed * Message.DeltaTime);
}