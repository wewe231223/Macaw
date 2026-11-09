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
        /**
         * @brief 용도별 바인딩 플래그와 부가 플래그를 채웁니다.
         * @param[in] Description 크기와 사용 방식이 채워진 버퍼 설명입니다.
         */
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            Description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            Description.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            Description.StructureByteStride = GetStride();
        }

        /**
         * @brief 이 버퍼가 담을 수 있는 최대 원소 수를 반환합니다.
         * @return 기본 구현은 바이트 크기가 32비트에 들어가는 원소 수입니다.
         */
        Core::Uint64 GetMaximumCount() const override {
            return std::min<Core::Uint64>(RenderCore::FGpuBuffer::GetMaximumCount(), Core::Uint64{1} << D3D11_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP);
        }

        /**
         * @brief 새 버퍼를 가리키는 뷰를 만듭니다.
         * @param[in] Device 뷰를 만들 장치입니다.
         * @param[in] Buffer 새로 만든 버퍼입니다.
         * @param[out] Error 뷰 생성 실패 원인입니다.
         * @return 성공하면 true이며, 실패하면 기존 뷰를 유지해야 합니다.
         */
        bool CreateViews(ID3D11Device* Device, ID3D11Buffer* Buffer, Core::FString& Error) override {
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View{};

            if (FAILED(Device->CreateShaderResourceView(Buffer, nullptr, View.GetAddressOf()))) {
                Error = "Structured buffer shader resource view creation failed";

                return false;
            }

            mShaderResourceView = std::move(View);

            return true;
        }

        /**
         * @brief 파생 클래스가 소유한 뷰를 해제합니다.
         */
        void ReleaseViews() override {
            mShaderResourceView.Reset();
        }

    private:
        /**
         * @brief 구조화 버퍼의 셰이더 리소스 뷰를 교체합니다.
         * @param[in] Context GPU 명령을 제출할 D3D11 장치 컨텍스트입니다.
         * @param[in] Stages 연결하거나 해제할 셰이더 단계입니다.
         * @param[in] Slot 조회하거나 바꿀 머티리얼 슬롯입니다.
         * @param[in] View 화면 크기와 뷰 투영 행렬입니다.
         */
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
