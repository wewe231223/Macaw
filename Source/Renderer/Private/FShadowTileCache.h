/**
 * @file FShadowTileCache.h
 * @brief 그림자 아틀라스 타일의 깊이를 다시 그릴지 판단하는 캐시의 키와 스탬프를 정의합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/FRevision.h"
#include "Core/Guid.h"
#include "Core/Math/FMatrix.h"
#include "Core/TRevisionCache.h"
#include "FShadowAtlasAllocator.h"

#include <variant>

namespace Renderer {

    /**
     * @brief 장면·광원·면으로 그림자 맵 하나를 구분하는 캐시 키입니다.
     */
    struct FShadowTileKey {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FShadowTileKey);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowTileKey);

        /**
         * @brief 두 키가 같은 그림자 맵을 가리키는지 비교합니다.
         * @param[in] Other 비교할 키입니다.
         * @return 장면·광원·면이 모두 같으면 true입니다.
         */
        bool operator==(const Renderer::FShadowTileKey& Other) const = default;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 그림자 맵을 소유한 장면의 고유 번호입니다.
         */
        Core::FRevision mScene{};
        ReflectProperty(mScene, "Scene", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 그림자를 만드는 광원 컴포넌트의 GUID입니다.
         */
        Core::FGuid mLight{};
        ReflectProperty(mLight, "Light", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 점광원 큐브 면 또는 방향광 캐스케이드 번호입니다.
         */
        Core::Uint32 mFace{};
        ReflectProperty(mFace, "Face", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief FShadowTileKey의 해시 함수 객체입니다.
     */
    struct FShadowTileKeyHash {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FShadowTileKeyHash);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowTileKeyHash);

        /**
         * @brief 장면 번호·광원 GUID·면 번호를 섞어 해시를 계산합니다.
         * @param[in] Key 해시할 키입니다.
         * @return 해시 값입니다.
         */
        std::size_t operator()(const Renderer::FShadowTileKey& Key) const noexcept;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();
    };

    /**
     * @brief 아틀라스 타일의 깊이 내용을 결정하는 입력 묶음입니다.
     * @details 바이어스처럼 샘플링에만 쓰이는 값은 깊이 내용과 무관하므로 포함하지 않습니다.
     */
    struct FShadowTileStamp {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FShadowTileStamp);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowTileStamp);

        /**
         * @brief 두 스탬프의 모든 입력이 같은지 비교합니다.
         * @param[in] Other 비교할 스탬프입니다.
         * @return 모든 입력이 같으면 true입니다.
         */
        bool operator==(const Renderer::FShadowTileStamp& Other) const = default;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 깊이를 기록한 보호 여백 포함 아틀라스 블록입니다.
         */
        Renderer::FShadowAtlasBlock mBlock{};
        ReflectProperty(mBlock, "Block", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이를 기록할 때 사용한 광원 뷰 투영 행렬입니다.
         */
        Core::FMatrix mViewProjection{};
        ReflectProperty(mViewProjection, "ViewProjection", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이를 기록할 때의 장면 캐스터 리비전입니다.
         */
        Core::FRevision mCasters{};
        ReflectProperty(mCasters, "Casters", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이를 기록한 아틀라스 텍스처의 생성 리비전입니다.
         */
        Core::FRevision mAtlas{};
        ReflectProperty(mAtlas, "Atlas", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 깊이를 기록한 깊이 전용 정점 셰이더의 리비전입니다.
         */
        Core::Uint64 mDepthShader{};
        ReflectProperty(mDepthShader, "DepthShader", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 아틀라스 타일의 깊이 내용이 현재 입력과 같은지 기억하는 캐시입니다.
     * @details 결과는 아틀라스 텍스처 안에 있으므로 값은 보관하지 않습니다.
     */
    using FShadowTileCache = Core::TRevisionCache<Renderer::FShadowTileKey, Renderer::FShadowTileStamp, std::monostate, Renderer::FShadowTileKeyHash, Core::FFrameAgeEviction>;

}
