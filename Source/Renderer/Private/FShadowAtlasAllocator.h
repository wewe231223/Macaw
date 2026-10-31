/**
 * @file FShadowAtlasAllocator.h
 * @brief 정사각형 영역을 사분할하여 그림자 아틀라스의 공간을 할당합니다.
 * @ingroup Renderer
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

#include <array>
#include <optional>

namespace Renderer {

    /**
     * @brief 그림자 아틀라스에 할당된 정사각형 영역의 픽셀 좌표와 크기입니다.
     * @ingroup Renderer
     */
    struct FShadowAtlasBlock {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FShadowAtlasBlock);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowAtlasBlock);

        /**
         * @brief 두 블록이 같은 위치와 크기인지 비교합니다.
         * @param[in] Other 비교할 블록입니다.
         * @return 위치와 크기가 모두 같으면 true입니다.
         */
        bool operator==(const Renderer::FShadowAtlasBlock& Other) const = default;

        /**
         * @brief 두 블록의 픽셀 영역이 겹치는지 검사합니다.
         * @param[in] Other 검사할 블록입니다.
         * @return 한 픽셀이라도 겹치면 true입니다.
         */
        bool Overlaps(const Renderer::FShadowAtlasBlock& Other) const;

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

    public:
        /**
         * @brief 할당 블록 왼쪽 가장자리의 픽셀 좌표입니다.
         */
        Core::Uint32 mX{};
        ReflectProperty(mX, "X", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 할당 블록 위쪽 가장자리의 픽셀 좌표입니다.
         */
        Core::Uint32 mY{};
        ReflectProperty(mY, "Y", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 할당 블록 한 변의 픽셀 수입니다.
         */
        Core::Uint32 mSize{};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

    /**
     * @brief 정사각형 영역을 사분할하여 그림자 아틀라스의 공간을 할당합니다.
     * @ingroup Renderer
     */
    class FShadowAtlasAllocator {
    public:
        /**
         * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
         */
        ReflectTypes(FShadowAtlasAllocator);

    private:
        /**
         * @brief 그림자 아틀라스의 영역과 네 자식 노드의 점유 상태입니다.
         */
        struct FNode {
        public:
            /**
             * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
             */
            ReflectTypes(FNode);

        public:
            /**
             * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
             * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
             */
            ReflectStruct(FNode);

        private:
            /**
             * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
             */
            ReflectMembers();

        public:
            /**
             * @brief 이 노드가 담당하는 아틀라스 정사각형 영역입니다.
             */
            Renderer::FShadowAtlasBlock mBlock{};
            ReflectProperty(mBlock, "Block", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 사분할된 네 자식 노드의 배열 인덱스입니다.
             */
            std::array<Core::Uint32, 4> mChildren{};
            ReflectProperty(mChildren, "Children", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 이 노드가 네 자식 영역으로 분할되었는지 나타냅니다.
             */
            bool mSplit{};
            ReflectProperty(mSplit, "Split", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

            /**
             * @brief 이 노드의 전체 영역이 할당되었는지 나타냅니다.
             */
            bool mOccupied{};
            ReflectProperty(mOccupied, "Occupied", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
        };

    public:
        /**
         * @brief 지정한 크기의 아틀라스 전체를 빈 영역으로 초기화합니다.
         * @param[in] Size 아틀라스 한 변의 픽셀 수이며 2의 거듭제곱 크기를 사용합니다.
         */
        explicit FShadowAtlasAllocator(Core::Uint32 Size = 16384);

    public:
        /**
         * @brief 이 타입의 정적 구조체 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FShadowAtlasAllocator);

        /**
         * @brief 모든 할당을 해제하고 아틀라스 전체를 하나의 빈 영역으로 되돌립니다.
         */
        void Reset();

        /**
         * @brief 요청한 크기의 비어 있는 정사각형 아틀라스 영역을 할당합니다.
         * @param[in] Size 할당할 한 변의 픽셀 수이며 0이 아닌 2의 거듭제곱이어야 합니다.
         * @return 할당된 블록이며 크기가 유효하지 않거나 공간이 없으면 std::nullopt입니다.
         */
        std::optional<Renderer::FShadowAtlasBlock> Allocate(Core::Uint32 Size);

    private:
        /**
         * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
         */
        ReflectMembers();

        /**
         * @brief 지정한 노드에서 빈 영역을 찾고 필요하면 사분할하여 할당합니다.
         * @param[in] Index 할당을 시도할 쿼드트리 노드의 인덱스입니다.
         * @param[in] Size 할당할 한 변의 픽셀 수이며 0이 아닌 2의 거듭제곱이어야 합니다.
         * @return 할당된 블록이며 해당 하위 트리에 공간이 없으면 std::nullopt입니다.
         */
        std::optional<Renderer::FShadowAtlasBlock> AllocateNode(Core::Uint32 Index, Core::Uint32 Size);

    private:
        /**
         * @brief 아틀라스 한 변의 픽셀 수입니다.
         */
        Core::Uint32 mSize{};
        ReflectProperty(mSize, "Size", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 아틀라스 사분할 트리의 노드 저장소입니다.
         */
        Core::TArray<FNode> mNodes{};
        ReflectProperty(mNodes, "Nodes", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
