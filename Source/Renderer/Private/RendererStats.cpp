/**
 * @file RendererStats.cpp
 * @brief 렌더링과 그림자 패스의 드로·바인딩·메모리 통계 항목입니다.
 * @ingroup Renderer
 */

#include "RendererStats.h"

/**
 * @brief 렌더링과 그림자 패스의 드로·바인딩·메모리 통계 항목입니다.
 */
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
