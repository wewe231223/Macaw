/**
 * @file TVertexBuffer.h
 * @brief 입력 조립 단계가 읽는 정점 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/FGpuBuffer.h"

#include <algorithm>
#include <span>

namespace RenderCore {

    /**
     * @brief 입력 조립 단계가 읽는 정점 버퍼입니다.
     * @tparam T 정점 하나의 타입입니다.
     * @tparam Update CPU 데이터를 기록하는 방식입니다.
     */
    template <typename T, RenderCore::EBufferUpdate Update = RenderCore::EBufferUpdate::Dynamic>
    class TVertexBuffer : public RenderCore::FGpuBuffer {
    public:
        /**
         * @brief T 크기로 설정한 빈 버퍼를 생성합니다.
         */
        TVertexBuffer()
            : RenderCore::FGpuBuffer{static_cast<Core::Uint32>(sizeof(T)), Update} {
        }

        /**
         * @brief 필요한 경우 버퍼를 재할당하고 데이터를 업로드합니다.
         * @param[in] Context 업로드할 장치 컨텍스트입니다.
         * @param[in] Data 비어 있지 않은 연속된 원소입니다.
         * @param[out] Error 할당 또는 기록 실패 원인입니다.
         * @return 업로드에 성공하면 true입니다.
         */
        bool Upload(ID3D11DeviceContext* Context, std::span<const T> Data, Core::FString& Error)
            requires(Update != RenderCore::EBufferUpdate::Immutable) {
            return UploadBytes(Context, std::as_bytes(Data), Error);
        }

        /**
         * @brief 데이터 크기에 꼭 맞는 불변 버퍼를 만듭니다.
         * @param[in] Device 자원을 만들 장치입니다.
         * @param[in] Data 비어 있지 않은 연속된 원소입니다.
         * @param[out] Error 데이터 오류 또는 할당 실패 원인입니다.
         * @return 같은 장치에 버퍼가 이미 있거나 생성에 성공하면 true입니다.
         */
        bool Create(ID3D11Device* Device, std::span<const T> Data, Core::FString& Error)
            requires(Update == RenderCore::EBufferUpdate::Immutable) {
            return CreateFromBytes(Device, std::as_bytes(Data), Error);
        }

        /**
         * @brief 입력 조립 단계에 버퍼를 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Slot 입력 슬롯입니다.
         * @param[in] ElementsPerVertex 정점 하나가 차지하는 T의 개수입니다.
         */
        void Bind(ID3D11DeviceContext* Context, Core::Uint32 Slot, Core::Uint32 ElementsPerVertex = 1) const {
            ID3D11Buffer* Buffer{GetBuffer()};
            const UINT Stride{GetStride() * ElementsPerVertex};
            const UINT Offset{};
            Context->IASetVertexBuffers(Slot, 1, &Buffer, &Stride, &Offset);
        }

        /**
         * @brief 입력 슬롯의 버퍼 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Slot 입력 슬롯입니다.
         */
        static void Unbind(ID3D11DeviceContext* Context, Core::Uint32 Slot) {
            ID3D11Buffer* Buffer{};
            const UINT Stride{};
            const UINT Offset{};
            Context->IASetVertexBuffers(Slot, 1, &Buffer, &Stride, &Offset);
        }

    protected:
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        }

        Core::Uint64 GetGrowthCapacity(Core::Uint64 Count) const override {
            return RenderCore::FGpuBuffer::GetGrowthCapacity(std::max<Core::Uint64>(Count, 256));
        }
    };

}
