#pragma once

#include "Core/Stats.h"

namespace RendererStats {
    extern FStat Triangles;
    extern FStat PipelineBinds;
    extern FStat MaterialBinds;
    extern FStat MeshBinds;
    extern FStat ShadowMaps;
    extern FStat ShadowDrawCalls;
    extern FStat ShadowReducedLights;
    extern FStat ShadowDroppedLights;
    extern FStat ShadowAtlasBytes;
    extern FStat ShadowAllocatedBytes;
    extern FStat DirectionalLights;
    extern FStat PointLights;
    extern FStat SpotLights;
}
