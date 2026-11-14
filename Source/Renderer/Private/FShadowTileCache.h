/**
 * @file FShadowTileCache.h
 * @brief 그림자 아틀라스 타일의 깊이를 다시 그릴지 판단하는 캐시의 키와 스탬프를 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/Types.h"
#include "Core/ReflectionMacros.h"
#include "Core/FRevision.h"
#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"
#include "Core/TRevisionCache.h"
#include "Engine/EShadowBiasPreset.h"
#include "FShadowAtlasAllocator.h"

#include <variant>

namespace Renderer {

    /**
     * @brief 장면·광원·면으로 그림자 맵 하나를 구분하는 캐시 키입니다.
     */
    struct FShadowTileKey {
    public:

        /**
         * @brief 두 키가 같은 그림자 맵을 가리키는지 비교합니다.
         * @param[in] Other 비교할 키입니다.
         * @return 장면·광원·면이 모두 같으면 true입니다.
         */
        bool operator==(const Renderer::FShadowTileKey& Other) const = default;

    public:

        /**
         * @brief 그림자 맵을 소유한 장면의 고유 번호입니다.
         */
        Core::FRevision mScene{};

        /**
         * @brief 그림자를 만드는 광원 컴포넌트의 GUID입니다.
         */
        Core::FGuid mLight{};

        /**
         * @brief 점광원 큐브 면 또는 방향광 캐스케이드 번호입니다.
         */
        Core::Index_t mFace{};
    };

    /**
     * @brief FShadowTileKey의 해시 함수 객체입니다.
     */
    struct FShadowTileKeyHash {
    public:

        /**
         * @brief 장면 번호·광원 GUID·면 번호를 섞어 해시를 계산합니다.
         * @param[in] Key 해시할 키입니다.
         * @return 해시 값입니다.
         */
        std::size_t operator()(const Renderer::FShadowTileKey& Key) const noexcept;
    };

    /**
     * @brief 아틀라스 타일의 깊이 내용을 결정하는 입력 묶음입니다.
     */
    struct FShadowTileStamp {
    public:

        /**
         * @brief 두 스탬프의 모든 입력이 같은지 비교합니다.
         * @param[in] Other 비교할 스탬프입니다.
         * @return 모든 입력이 같으면 true입니다.
         */
        bool operator==(const Renderer::FShadowTileStamp& Other) const = default;

    public:

        /**
         * @brief 깊이를 기록한 보호 여백 포함 아틀라스 블록입니다.
         */
        Renderer::FShadowAtlasBlock mBlock{};

        /**
         * @brief 깊이를 기록할 때 사용한 광원 뷰 투영 행렬입니다.
         */
        Core::FMatrix mViewProjection{};

        /**
         * @brief 깊이를 기록할 때의 장면 캐스터 리비전입니다.
         */
        Core::FRevision mCasters{};

        /**
         * @brief 깊이를 기록한 아틀라스 텍스처의 생성 리비전입니다.
         */
        Core::FRevision mAtlas{};

        /**
         * @brief 깊이를 기록한 깊이 전용 정점 셰이더의 리비전입니다.
         */
        Core::Uint64 mDepthShader{};

        /**
         * @brief 타일을 그릴 때 사용할 깊이 바이어스 단계입니다.
         */
        Engine::EShadowBiasPreset mBiasPreset{Engine::DefaultShadowBiasPreset};

        /**
         * @brief 타일을 그릴 때 사용할 경사 바이어스 단계입니다.
         */
        Engine::EShadowBiasPreset mSlopeBiasPreset{Engine::DefaultShadowBiasPreset};
    };

    /**
     * @brief 아틀라스 타일의 깊이 내용이 현재 입력과 같은지 기억하는 캐시입니다.
     * @details 결과는 아틀라스 텍스처 안에 있으므로 값은 보관하지 않습니다.
     */
    using FShadowTileCache = Core::TRevisionCache<Renderer::FShadowTileKey, Renderer::FShadowTileStamp, std::monostate, Renderer::FShadowTileKeyHash, Core::FFrameAgeEviction>;

}
