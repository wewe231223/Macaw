#include "PCH.h"
#include "UWorld.h"

const std::vector<std::unique_ptr<AActor>>& UWorld::GetObjects() const
{
    return Actors;
}

FRenderProbe UWorld::BuildRenderProbe() const
{
    FRenderProbe Probe;

    for (const UStaticMeshComponent* MeshComponent : StaticMeshComponents)
    {
            ActorProbe Actor;

            Actor.World =
                MeshComponent->GetTransform().GetWorldMatrix();

            Actor.MeshHandle =
                MeshComponent->GetMeshHandle();

            Actor.MaterialHandle =
                MeshComponent->GetMaterialHandle();

            Actor.PipelineHandle =
                MeshComponent->GetPipelineHandle();

            Probe.ActorProbes.push_back(Actor);
        
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

