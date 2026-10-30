/**
 * @file EngineStats.cpp
 * @brief 엔진·월드·에셋 처리의 시간과 개수를 측정하는 통계 항목입니다.
 * @ingroup Engine
 */

#include "Engine/EngineStats.h"

/**
 * @brief 엔진·월드·에셋 처리의 시간과 개수를 측정하는 통계 항목입니다.
 */
namespace EngineStats {
    FStat EngineTick{"unit", "Engine tick CPU", EStatUnit::Milliseconds};
    FStat Synchronize{"unit", "Scene synchronization CPU", EStatUnit::Milliseconds};
    FStat WorldTick{"world", "World tick CPU (inclusive)", EStatUnit::Milliseconds};
    FStat RenderUpdates{"world", "Render updates CPU", EStatUnit::Milliseconds};
    FStat Raycast{"world", "Raycast CPU", EStatUnit::Milliseconds};
    FStat ActorTicks{"game", "Actor ticks"};
    FStat ComponentTicks{"game", "Component ticks"};
    FStat WorldTicks{"game", "World ticks"};
    FStat Worlds{"scene", "Worlds (engine)", EStatUnit::Count, EStatMode::Gauge};
    FStat Actors{"scene", "Actors (engine worlds)", EStatUnit::Count, EStatMode::Gauge};
    FStat Components{"scene", "Components (engine worlds)", EStatUnit::Count, EStatMode::Gauge};
    FStat SharedGeometry{"memory", "Shared CPU meshes", EStatUnit::Bytes, EStatMode::Gauge};
    FStat MeshLoad{"assets", "Mesh load CPU (incl. cache)", EStatUnit::Milliseconds};
    FStat TextureLoad{"assets", "Texture load CPU (incl. cache)", EStatUnit::Milliseconds};
    FStat MaterialLoad{"assets", "Material load CPU (inclusive)", EStatUnit::Milliseconds};
    FStat MeshImport{"assets", "Mesh import CPU (inclusive)", EStatUnit::Milliseconds};
    FStat TextureImport{"assets", "Texture import CPU (inclusive)", EStatUnit::Milliseconds};
    FStat AssetCacheHits{"assets", "Cache hits"};
}
