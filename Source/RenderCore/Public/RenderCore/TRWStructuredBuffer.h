/**
 * @file TRWStructuredBuffer.h
 * @brief GPU가 쓰고 읽는 구조화 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "RenderCore/TStructuredBuffer.h"

#include <utility>

namespace RenderCore {

    /**
     * @brief HLSL RWStructuredBuffer로 쓰고 StructuredBuffer로 읽는 버퍼입니다.
     * @tparam T 원소 하나의 타입이며 HLSL 구조체와 배치가 일치해야 합니다.
     * @note Direct3D 11은 DYNAMIC 버퍼의 순서 없는 접근을 허용하지 않아 갱신 방식을 Default로 고정합니다.
     */
    template <typename T>
    class TRWStructuredBuffer : public RenderCore::TStructuredBuffer<T, RenderCore::EBufferUpdate::Default> {
    public:
        /**
         * @brief 컴퓨트 셰이더 단계에 순서 없는 접근 뷰를 연결합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Slot 순서 없는 접근 슬롯(u 레지스터)입니다.
         */
        void BindOutput(ID3D11DeviceContext* Context, Core::Uint32 Slot) const {
            ID3D11UnorderedAccessView* View{mUnorderedAccessView.Get()};
            Context->CSSetUnorderedAccessViews(Slot, 1, &View, nullptr);
        }

        /**
         * @brief 컴퓨트 셰이더 단계의 순서 없는 접근 슬롯 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Slot 순서 없는 접근 슬롯(u 레지스터)입니다.
         */
        void UnbindOutput(ID3D11DeviceContext* Context, Core::Uint32 Slot) const {
            ID3D11UnorderedAccessView* View{};
            Context->CSSetUnorderedAccessViews(Slot, 1, &View, nullptr);
        }

    protected:
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            FBase::FillDescription(Description);
            Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        bool CreateViews(ID3D11Device* Device, ID3D11Buffer* Buffer, Core::FString& Error) override {
            Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> View{};

            // 기반 클래스가 뷰를 교체하기 전에 실패할 수 있는 작업을 끝내 두 뷰가 다른 버퍼를 가리키지 않게 합니다.
            if (FAILED(Device->CreateUnorderedAccessView(Buffer, nullptr, View.GetAddressOf()))) {
                Error = "Structured buffer unordered access view creation failed";
                return false;
            }

            if (!FBase::CreateViews(Device, Buffer, Error)) {
                return false;
            }

            mUnorderedAccessView = std::move(View);
            return true;
        }

        void ReleaseViews() override {
            FBase::ReleaseViews();
            mUnorderedAccessView.Reset();
        }

    private:
        using FBase = RenderCore::TStructuredBuffer<T, RenderCore::EBufferUpdate::Default>;

        /**
         * @brief 버퍼 전체에 접근하는 순서 없는 접근 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mUnorderedAccessView{};
    };

}
