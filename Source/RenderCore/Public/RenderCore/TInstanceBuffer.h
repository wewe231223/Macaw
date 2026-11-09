/**
 * @file TInstanceBuffer.h
 * @brief 인스턴스별 데이터를 담는 정점 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/TVertexBuffer.h"

namespace RenderCore {

    /**
     * @brief D3D11_INPUT_PER_INSTANCE_DATA 입력으로 읽는 인스턴스 버퍼입니다.
     * @tparam T 인스턴스 하나의 데이터 타입입니다.
     * @tparam Update CPU 데이터를 기록하는 방식입니다.
     */
    template <typename T, RenderCore::EBufferUpdate Update = RenderCore::EBufferUpdate::Dynamic>
    class TInstanceBuffer : public RenderCore::TVertexBuffer<T, Update> {
    protected:
        // 인스턴스 데이터는 원소가 커서 정점 버퍼의 최소 용량을 두지 않습니다.

        /**
         * @brief 재할당할 때 확보할 원소 수를 정합니다.
         * @param[in] Count 필요한 원소 수입니다.
         * @return 기본 구현은 Count 이상의 가장 작은 2의 거듭제곱입니다.
         */
        Core::Uint64 GetGrowthCapacity(Core::Uint64 Count) const override {
            return RenderCore::FGpuBuffer::GetGrowthCapacity(Count);
        }
    };

}
