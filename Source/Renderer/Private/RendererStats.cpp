#include "RendererStats.h"

namespace RendererStats {
    FStat Triangles{"render", "Scene/editor triangles"};
    FStat PipelineBinds{"render", "Mesh pipeline binds"};
    FStat MaterialBinds{"render", "Mesh material binds"};
    FStat MeshBinds{"render", "Mesh buffer binds"};
    FStat ShadowMaps{"shadow", "Shadow maps (sum of views)"};
    FStat ShadowDrawCalls{"shadow", "Shadow draw calls"};
    FStat ShadowReducedLights{"shadow", "Reduced lights (sum of views)"};
    FStat ShadowDroppedLights{"shadow", "Unallocated lights (sum of views)"};
    FStat ShadowAtlasBytes{"shadow", "Atlas resources (last view)", EStatUnit::Bytes, EStatMode::Gauge};
    FStat ShadowAllocatedBytes{"shadow", "Allocated tiles (last view)", EStatUnit::Bytes, EStatMode::Gauge};
    FStat DirectionalLights{"shadow", "Directional lights (sum of views)"};
    FStat PointLights{"shadow", "Point lights (sum of views)"};
    FStat SpotLights{"shadow", "Spot lights (sum of views)"};
}
