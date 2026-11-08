#pragma once

#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"

namespace Asset {
    class UTexture2D;
}

namespace Engine {

    struct FDecalSceneProxy {
        Core::FGuid mId{};
        Core::FMatrix mDecalToWorld{};
        Core::FMatrix mWorldToDecal{};
        Asset::UTexture2D* mTexture{};
        float mOpacity{1.0f};
        Core::Int32 mSortOrder{};
    };

}
