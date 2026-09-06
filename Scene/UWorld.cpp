#include "PCH.h"
#include "UWorld.h"
#include "UCamera.h"
#include "URenderableObject.h"

const std::vector<std::unique_ptr<UObject>>& UWorld::GetObjects() const
{
    return Objects;
}

FRenderProbe UWorld::BuildRenderProbe() const
{
    FRenderProbe Probe;

    for (const URenderableObject* Renderable : RenderableObjects)
    {
            ActorProbe Actor;

            Actor.World =
                Renderable->GetTransform().GetWorldMatrix();

            Actor.MeshHandle =
                Renderable->GetMeshHandle();

            Actor.MaterialHandle =
                Renderable->GetMaterialHandle();

            Actor.PipelineHandle =
                Renderable->GetPipelineHandle();

            Probe.ActorProbes.push_back(Actor);
        
    }

    return Probe;
}