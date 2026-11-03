/**
 * @file TConstantBuffer.h
 * @brief T 하나를 담는 셰이더 상수 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/EShaderStage.h"
#include "RenderCore/FGpuBuffer.h"

namespace RenderCore {

    /**
     * @brief HLSL cbuffer로 읽는 상수 버퍼입니다.
     * @tparam T 상수 구조체이며 크기가 16바이트의 배수여야 합니다.
     */
    template <typename T>
    class TConstantBuffer : public RenderCore::FGpuBuffer {
    public:
        /**
         * @brief T 크기로 설정한 빈 버퍼를 생성합니다.
         */
        TConstantBuffer()
            : RenderCore::FGpuBuffer{static_cast<Core::Uint32>(sizeof(T)), RenderCore::EBufferUpdate::Default} {
            static_assert(sizeof(T) % 16 == 0, "Constant buffer size must be a multiple of 16 bytes");
        }

        /**
         * @brief 버퍼가 없으면 만듭니다.
         * @param[in] Device 자원을 만들 장치입니다.
         * @param[out] Error 할당 실패 원인입니다.
         * @return 버퍼가 있거나 생성에 성공하면 true입니다.
         */
        bool Create(ID3D11Device* Device, Core::FString& Error) {
            return Reserve(Device, 1, Error);
        }

        /**
         * @brief 상수 전체를 갱신합니다.
         * @param[in] Context 갱신할 장치 컨텍스트입니다.
         * @param[in] Data 새 상수 값입니다.
         * @note Create에 성공한 뒤에 호출해야 합니다.
         */
        void Update(ID3D11DeviceContext* Context, const T& Data) const {
            Context->UpdateSubresource(GetBuffer(), 0, nullptr, &Data, 0, 0);
        }

        /**
         * @brief 지정한 셰이더 단계에 버퍼를 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 연결할 셰이더 단계입니다.
         * @param[in] Slot 상수 버퍼 슬롯(b 레지스터)입니다.
         */
        void Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot) const {
            SetBuffer(Context, Stages, Slot, GetBuffer());
        }

        /**
         * @brief 지정한 셰이더 단계의 슬롯 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 해제할 셰이더 단계입니다.
         * @param[in] Slot 상수 버퍼 슬롯(b 레지스터)입니다.
         */
        static void Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot) {
            SetBuffer(Context, Stages, Slot, nullptr);
        }

    protected:
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }

        Core::Uint64 GetMaximumCount() const override {
            return 1;
        }

    private:
        static void SetBuffer(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot, ID3D11Buffer* Buffer) {
            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Vertex)) {
                Context->VSSetConstantBuffers(Slot, 1, &Buffer);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Pixel)) {
                Context->PSSetConstantBuffers(Slot, 1, &Buffer);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Compute)) {
                Context->CSSetConstantBuffers(Slot, 1, &Buffer);
            }
        }
    };

}
