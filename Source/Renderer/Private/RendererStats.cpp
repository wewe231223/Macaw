/**
 * @file RendererStats.cpp
 * @brief 렌더링과 그림자 패스의 드로·바인딩·메모리 통계 항목입니다.
 * @ingroup Renderer
 */

#include "RendererStats.h"

namespace Renderer {

    /**
     * @brief 렌더링과 그림자 패스의 드로·바인딩·메모리 통계 항목입니다.
     */
    namespace RendererStats {
        Core::FStat Triangles{"render", "Scene/editor triangles"};
        Core::FStat PipelineBinds{"render", "Mesh pipeline binds"};
        Core::FStat MaterialBinds{"render", "Mesh material binds"};
        Core::FStat MeshBinds{"render", "Mesh buffer binds"};
        Core::FStat ShadowMaps{"shadow", "Shadow maps (sum of views)"};
        Core::FStat ShadowDrawCalls{"shadow", "Shadow draw calls"};
        Core::FStat ShadowReducedLights{"shadow", "Reduced lights (sum of views)"};
        Core::FStat ShadowDroppedLights{"shadow", "Unallocated lights (sum of views)"};
        Core::FStat ShadowAtlasBytes{"shadow", "Atlas resources (last view)", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};
        Core::FStat ShadowAllocatedBytes{"shadow", "Allocated tiles (last view)", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};
        Core::FStat DirectionalLights{"shadow", "Directional lights (sum of views)"};
        Core::FStat PointLights{"shadow", "Point lights (sum of views)"};
        Core::FStat SpotLights{"shadow", "Spot lights (sum of views)"};
    }

}
