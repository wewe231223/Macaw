/**
 * @file TStructuredBuffer.h
 * @brief 셰이더가 읽는 구조화 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/EShaderStage.h"
#include "RenderCore/FGpuBuffer.h"

#include <algorithm>
#include <span>
#include <utility>

namespace RenderCore {

    /**
     * @brief HLSL StructuredBuffer로 읽는 원소 배열을 담는 버퍼입니다.
     * @tparam T 원소 하나의 타입이며 HLSL 구조체와 배치가 일치해야 합니다.
     * @tparam Update CPU 데이터를 기록하는 방식입니다.
     */
    template <typename T, RenderCore::EBufferUpdate Update = RenderCore::EBufferUpdate::Default>
    class TStructuredBuffer : public RenderCore::FGpuBuffer {
        static_assert(Update != RenderCore::EBufferUpdate::Immutable, "Immutable structured buffers are not supported yet");

    public:
        /**
         * @brief T 크기로 설정한 빈 버퍼를 생성합니다.
         */
        TStructuredBuffer()
            : RenderCore::FGpuBuffer{static_cast<Core::Uint32>(sizeof(T)), Update} {
        }

        /**
         * @brief 필요한 경우 버퍼를 재할당하고 원소를 업로드합니다.
         * @param[in] Context 업로드할 장치 컨텍스트입니다.
         * @param[in] Data 비어 있지 않은 연속된 원소입니다.
         * @param[out] Error 할당 또는 기록 실패 원인입니다.
         * @return 업로드에 성공하면 true입니다.
         */
        bool Upload(ID3D11DeviceContext* Context, std::span<const T> Data, Core::FString& Error) {
            return UploadBytes(Context, std::as_bytes(Data), Error);
        }

        /**
         * @brief 지정한 셰이더 단계에 셰이더 리소스 뷰를 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 연결할 셰이더 단계입니다.
         * @param[in] Slot 셰이더 리소스 슬롯(t 레지스터)입니다.
         */
        void Bind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot) const {
            SetView(Context, Stages, Slot, mShaderResourceView.Get());
        }

        /**
         * @brief 지정한 셰이더 단계의 슬롯 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Stages 해제할 셰이더 단계입니다.
         * @param[in] Slot 셰이더 리소스 슬롯(t 레지스터)입니다.
         */
        static void Unbind(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot) {
            SetView(Context, Stages, Slot, nullptr);
        }

    protected:
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            Description.StructureByteStride = GetStride();
        }

        Core::Uint64 GetMaximumCount() const override {
            return std::min<Core::Uint64>(RenderCore::FGpuBuffer::GetMaximumCount(), Core::Uint64{1} << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP);
        }

        bool CreateViews(ID3D11Device* Device, ID3D11Buffer* Buffer, Core::FString& Error) override {
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View{};

            if (FAILED(Device->CreateShaderResourceView(Buffer, nullptr, View.GetAddressOf()))) {
                Error = "Structured buffer shader resource view creation failed";
                return false;
            }

            mShaderResourceView = std::move(View);
            return true;
        }

        void ReleaseViews() override {
            mShaderResourceView.Reset();
        }

    private:
        static void SetView(ID3D11DeviceContext* Context, RenderCore::EShaderStage Stages, Core::Uint32 Slot, ID3D11ShaderResourceView* View) {
            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Vertex)) {
                Context->VSSetShaderResources(Slot, 1, &View);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Pixel)) {
                Context->PSSetShaderResources(Slot, 1, &View);
            }

            if (RenderCore::HasStage(Stages, RenderCore::EShaderStage::Compute)) {
                Context->CSSetShaderResources(Slot, 1, &View);
            }
        }

    private:
        /**
         * @brief 버퍼 전체를 읽는 셰이더 리소스 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> mShaderResourceView{};
    };

}
