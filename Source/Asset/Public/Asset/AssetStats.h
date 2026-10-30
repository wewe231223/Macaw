#pragma once

#include "Core/Stats.h"

namespace Asset {

    namespace AssetStats {
        /**
         * @brief 메시 로드 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat MeshLoad;

        /**
         * @brief 텍스처 로드 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat TextureLoad;

        /**
         * @brief 머티리얼 로드 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat MaterialLoad;

        /**
         * @brief 메시 임포트 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat MeshImport;

        /**
         * @brief 텍스처 임포트 처리 시간를 측정하는 통계 항목입니다.
         */
        extern Core::FStat TextureImport;

        /**
         * @brief 에셋 캐시 적중 횟수를 측정하는 통계 항목입니다.
         */
        extern Core::FStat AssetCacheHits;
    }

}
