#include "Asset/AssetStats.h"

namespace Asset {

    namespace AssetStats {
        Core::FStat MeshLoad{"assets", "Mesh load CPU (incl. cache)", Core::EStatUnit::Milliseconds};
        Core::FStat TextureLoad{"assets", "Texture load CPU (incl. cache)", Core::EStatUnit::Milliseconds};
        Core::FStat MaterialLoad{"assets", "Material load CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat MeshImport{"assets", "Mesh import CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat TextureImport{"assets", "Texture import CPU (inclusive)", Core::EStatUnit::Milliseconds};
        Core::FStat AssetCacheHits{"assets", "Cache hits"};
    }

}
