#pragma once

#include "Core/Stats.h"

namespace EngineStats {
    extern FStat EngineTick;
    extern FStat Synchronize;
    extern FStat WorldTick;
    extern FStat RenderUpdates;
    extern FStat Raycast;
    extern FStat ActorTicks;
    extern FStat ComponentTicks;
    extern FStat WorldTicks;
    extern FStat Worlds;
    extern FStat Actors;
    extern FStat Components;
    extern FStat SharedGeometry;
    extern FStat MeshLoad;
    extern FStat TextureLoad;
    extern FStat MaterialLoad;
    extern FStat MeshImport;
    extern FStat TextureImport;
    extern FStat AssetCacheHits;
}
