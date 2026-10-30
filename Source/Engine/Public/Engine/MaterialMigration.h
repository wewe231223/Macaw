#pragma once

#include "Core/Types.h"

namespace Asset {
    class FAssetManager;
}

namespace Engine::MaterialMigration {

    bool Migrate(Asset::FAssetManager& Assets, Core::FString& Error);

}
