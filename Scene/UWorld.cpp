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

    for (const auto& Object : Objects)
    {
        if (const auto* Renderable =
            dynamic_cast<const URenderableObject*>(Object.get()))   // 캐스팅 안쓰고...
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

        if (const auto* Camera =
            dynamic_cast<const UCamera*>(Object.get()))
        {
            Probe.MainCameraProbe.View =
                Camera->GetViewMatrix();

            Probe.MainCameraProbe.Projection =
                Camera->GetProjectionMatrix();

            Probe.MainCameraProbe.ViewProjection =
                Camera->GetViewProjectionMatrix();
        }
    }

    return Probe;
}