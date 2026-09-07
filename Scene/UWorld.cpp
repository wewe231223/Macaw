#include "PCH.h"
#include "UWorld.h"

#include <algorithm>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"

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
    CollisionComponents.push_back(Component);
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

AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) {
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
