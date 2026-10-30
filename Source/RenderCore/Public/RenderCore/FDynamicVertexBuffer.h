/**
 * @file FDynamicVertexBuffer.h
 * @brief 용량을 재사용하는 Direct3D 11 동적 정점 버퍼를 선언합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/ReflectionMacros.h"
#include "Core/Types.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <cstddef>
#include <span>

namespace RenderCore {

    /**
     * @brief 정점 버퍼의 용량, 재할당과 WRITE_DISCARD 업로드를 관리합니다.
     */
    class FDynamicVertexBuffer {
    public:
        /**
         * @brief 리플렉션 소유 타입입니다.
         */
        ReflectTypes(FDynamicVertexBuffer);

    public:
        /**
         * @brief GPU 자원이 없는 빈 버퍼를 생성합니다.
         */
        FDynamicVertexBuffer() = default;

        /**
         * @brief 소유한 GPU 버퍼를 해제합니다.
         */
        ~FDynamicVertexBuffer() = default;

        /**
         * @brief GPU 버퍼 소유 상태의 복사를 금지합니다.
         */
        FDynamicVertexBuffer(const RenderCore::FDynamicVertexBuffer&) = delete;

        /**
         * @brief GPU 버퍼 소유 상태의 복사 대입을 금지합니다.
         */
        RenderCore::FDynamicVertexBuffer& operator=(const RenderCore::FDynamicVertexBuffer&) = delete;

    public:
        /**
         * @brief 버퍼의 리플렉션 메타데이터를 반환합니다.
         * @return 등록된 구조체 메타데이터의 비소유 주소입니다.
         */
        ReflectStruct(FDynamicVertexBuffer);

        /**
         * @brief 필요한 경우 버퍼를 재할당하고 정점 바이트를 업로드합니다.
         * @param[in] Context 업로드할 장치 컨텍스트입니다.
         * @param[in] Data 연속된 정점 바이트입니다.
         * @param[in] Stride 정점 하나의 바이트 수입니다.
         * @param[in] Capacity 재할당할 때 확보할 정점 수이며 Data의 정점 수 이상이어야 합니다.
         * @param[out] Error 할당 또는 매핑 실패 원인입니다.
         * @return 업로드에 성공하면 true입니다.
         * @note 용량이 충분하면 기존 버퍼를 유지하므로 증가 정책은 호출자가 정합니다.
         */
        bool Upload(ID3D11DeviceContext* Context, std::span<const std::byte> Data, Core::Uint32 Stride, Core::Uint32 Capacity, Core::FString& Error);

        /**
         * @brief 소유한 GPU 버퍼와 용량 정보를 초기화합니다.
         */
        void Reset();

        /**
         * @brief 바인딩에 사용할 버퍼의 비소유 주소를 반환합니다.
         * @return 할당한 정점 버퍼이며 미할당 상태이면 nullptr입니다.
         */
        ID3D11Buffer* GetBuffer() const;

        /**
         * @brief 현재 확보한 GPU 버퍼 크기를 반환합니다.
         * @return 정점 용량에 정점 크기를 곱한 바이트 수입니다.
         */
        std::size_t GetResourceBytes() const;

    private:
        /**
         * @brief 리플렉션 등록 함수에 접근을 허용합니다.
         */
        ReflectMembers();

    private:
        /**
         * @brief 동적으로 기록하는 GPU 정점 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
        ReflectProperty(mBuffer, "Buffer", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 버퍼에 확보한 정점 수입니다.
         */
        Core::Uint32 mCapacity{};
        ReflectProperty(mCapacity, "Capacity", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);

        /**
         * @brief 정점 하나의 바이트 수입니다.
         */
        Core::Uint32 mStride{};
        ReflectProperty(mStride, "Stride", Core::EPropertyFlags::Transient | Core::EPropertyFlags::ReadOnly);
    };

}
