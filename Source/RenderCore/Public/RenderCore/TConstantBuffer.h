/**
 * @file TConstantBuffer.h
 * @brief T 하나를 담는 셰이더 상수 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
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
        void Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t Slot) const {
            SetBuffer(Context, Stages, Slot, GetBuffer());
        }

        /**
         * @brief 지정한 셰이더 단계의 슬롯 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 해제할 셰이더 단계입니다.
         * @param[in] Slot 상수 버퍼 슬롯(b 레지스터)입니다.
         */
        static void Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t Slot) {
            SetBuffer(Context, Stages, Slot, nullptr);
        }

    protected:
        /**
         * @brief 용도별 바인딩 플래그와 부가 플래그를 채웁니다.
         * @param[in] Description 크기와 사용 방식이 채워진 버퍼 설명입니다.
         */
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        }

        /**
         * @brief 이 버퍼가 담을 수 있는 최대 원소 수를 반환합니다.
         * @return 기본 구현은 바이트 크기가 32비트에 들어가는 원소 수입니다.
         */
        Core::Uint64 GetMaximumCount() const override {
            return 1;
        }

    private:
        /**
         * @brief 래핑하는 D3D11 버퍼와 원소 용량을 교체합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param[in] Stages 연결하거나 해제할 셰이더 단계입니다.
         * @param[in] Slot 조회하거나 바꿀 머티리얼 슬롯입니다.
         * @param[in] Buffer 래핑할 D3D11 버퍼의 COM 소유권입니다.
         */
        static void SetBuffer(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Index_t Slot, ID3D11Buffer* Buffer) {
            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Vertex)) {
                Context->VSSetConstantBuffers(static_cast<UINT>(Slot), 1, &Buffer);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Pixel)) {
                Context->PSSetConstantBuffers(static_cast<UINT>(Slot), 1, &Buffer);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Compute)) {
                Context->CSSetConstantBuffers(static_cast<UINT>(Slot), 1, &Buffer);
            }
        }
    };

}
