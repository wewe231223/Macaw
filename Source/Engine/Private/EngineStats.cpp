/**
 * @file EngineStats.cpp
 * @brief 엔진·월드·에셋 처리의 시간과 개수를 측정하는 통계 항목입니다.
 * @ingroup Engine
 */

#include "Engine/EngineStats.h"

namespace Engine {

    /**
     * @brief 엔진·월드·에셋 처리의 시간과 개수를 측정하는 통계 항목입니다.
     */
    namespace EngineStats {
        Core::FStat EngineTick{"unit", "Engine tick CPU", Core::EStatUnit::Milliseconds};
        Core::FStat Synchronize{"unit", "Scene synchronization CPU", Core::EStatUnit::Milliseconds};
        Core::FStat WorldTick{"world", "World tick CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat RenderUpdates{"world", "Render updates CPU", Core::EStatUnit::Milliseconds};
        Core::FStat Raycast{"world", "Raycast CPU", Core::EStatUnit::Milliseconds};
        Core::FStat ActorTicks{"game", "Actor ticks"};
        Core::FStat ComponentTicks{"game", "Component ticks"};
        Core::FStat WorldTicks{"game", "World ticks"};
        Core::FStat Worlds{"scene", "Worlds (engine)", Core::EStatUnit::Count, Core::EStatMode::Gauge};
        Core::FStat Actors{"scene", "Actors (engine worlds)", Core::EStatUnit::Count, Core::EStatMode::Gauge};
        Core::FStat Components{"scene", "Components (engine worlds)", Core::EStatUnit::Count, Core::EStatMode::Gauge};
        Core::FStat SharedGeometry{"memory", "Shared CPU meshes", Core::EStatUnit::Bytes, Core::EStatMode::Gauge};
        Core::FStat MeshLoad{"assets", "Mesh load CPU (incl. cache)", Core::EStatUnit::Milliseconds};
        Core::FStat TextureLoad{"assets", "Texture load CPU (incl. cache)", Core::EStatUnit::Milliseconds};
        Core::FStat MaterialLoad{"assets", "Material load CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat MeshImport{"assets", "Mesh import CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat TextureImport{"assets", "Texture import CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat AssetCacheHits{"assets", "Cache hits"};
    }

}
