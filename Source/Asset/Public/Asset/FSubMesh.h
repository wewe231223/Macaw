/**
 * @file FSubMesh.h
 * @brief 공유 정점 버퍼에서 재질별로 그릴 인덱스 구간입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/ReflectionMacros.h"

#include "Core/Types.h"

namespace Asset {

    /**
     * @brief 하나의 재질 슬롯을 사용하는 연속 삼각형 인덱스 구간입니다.
     */
    class FSubMesh {
    public:
        /**
         * @brief 비어 있는 구간을 구성합니다.
         */
        FSubMesh() = default;

        /**
         * @brief 공유 인덱스 버퍼의 구간과 재질을 지정합니다.
         * @param FirstIndex 첫 인덱스의 오프셋입니다.
         * @param IndexCount 삼각형 인덱스 개수입니다.
         * @param MaterialIndex 재질 슬롯의 인덱스입니다.
         */
        FSubMesh(Core::Uint32 FirstIndex, Core::Uint32 IndexCount, Core::Uint32 MaterialIndex);

    public:
        /**
         * @brief 첫 인덱스 오프셋을 반환합니다.
         * @return 공유 인덱스 버퍼의 오프셋입니다.
         */
        Core::Uint32 GetFirstIndex() const;

        /**
         * @brief 인덱스 개수를 반환합니다.
         * @return 삼각형 인덱스 개수입니다.
         */
        Core::Uint32 GetIndexCount() const;

        /**
         * @brief 재질 슬롯을 반환합니다.
         * @return 메시 재질 배열의 인덱스입니다.
         */
        Core::Uint32 GetMaterialIndex() const;

    private:
        /**
         * @brief 공유 인덱스 버퍼의 시작 오프셋입니다.
         */
        Core::Uint32 mFirstIndex{};

        /**
         * @brief 이 구간의 인덱스 개수입니다.
         */
        Core::Uint32 mIndexCount{};

        /**
         * @brief 이 구간에 할당된 재질 슬롯입니다.
         */
        Core::Uint32 mMaterialIndex{};
    };

}
