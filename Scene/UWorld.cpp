#include "PCH.h"
#include "UWorld.h"
#include "Core/Base/TypeInfo.h"
#include "Scene/Component/UStaticMeshComponent.h"

const std::vector<std::unique_ptr<AActor>>& UWorld::GetObjects() const
{
    return Actors;
}

const FRenderProbe& UWorld::BuildRenderProbe()
{
    RenderProbe.ActorProbes.clear();

    for (const UStaticMeshComponent* MeshComponent : StaticMeshComponents)
    {
        ActorProbe Actor;

        Actor.World = MeshComponent->GetTransform().GetWorldMatrix();
        Actor.MeshHandle = MeshComponent->GetMeshHandle();
        Actor.MaterialHandle = MeshComponent->GetMaterialHandle();
        Actor.PipelineHandle = MeshComponent->GetPipelineHandle();

        RenderProbe.ActorProbes.push_back(Actor);
    }

    if (Camera != nullptr)
    {
        RenderProbe.MainCameraProbe.View = Camera->GetViewMatrix();
        RenderProbe.MainCameraProbe.Projection = Camera->GetProjectionMatrix();
        RenderProbe.MainCameraProbe.ViewProjection = Camera->GetViewProjectionMatrix();
    }
    return RenderProbe;
}

void UWorld::RegisterComponent(UActorComponent* Component)
{
    if (auto* Mesh = dynamic_cast<UStaticMeshComponent*>(Component))
    {
        StaticMeshComponents.push_back(Mesh);
    }

    if (auto* CameraComponent = dynamic_cast<UCameraComponent*>(Component))
    {
        Camera = CameraComponent;
    }
}

//void UWorld::RegisterComponent(UActorComponent* Component)
//{
//    if (Component->GetTypeInfo()->IsA(UStaticMeshComponent::StaticTypeInfo()))
//    {
//        StaticMeshComponents.push_back(static_cast<UStaticMeshComponent*>(Component));
//    }
//
//    if (Component->GetTypeInfo()->IsA(UCameraComponent::StaticTypeInfo()))
//    {
//        Camera = static_cast<UCameraComponent*>(Component);
//    }
//}