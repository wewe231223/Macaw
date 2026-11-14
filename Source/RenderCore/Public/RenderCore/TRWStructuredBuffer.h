/**
 * @file TRWStructuredBuffer.h
 * @brief GPU가 쓰고 읽는 구조화 버퍼를 정의합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
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
        void BindOutput(ID3D11DeviceContext* Context, Core::Index_t Slot) const {
            ID3D11UnorderedAccessView* View{mUnorderedAccessView.Get()};

            Context->CSSetUnorderedAccessViews(static_cast<UINT>(Slot), 1, &View, nullptr);
        }

        /**
         * @brief 컴퓨트 셰이더 단계의 순서 없는 접근 슬롯 연결을 해제합니다.
         * @param[in] Context 바인딩할 장치 컨텍스트입니다.
         * @param[in] Slot 순서 없는 접근 슬롯(u 레지스터)입니다.
         */
        void UnbindOutput(ID3D11DeviceContext* Context, Core::Index_t Slot) const {
            ID3D11UnorderedAccessView* View{};

            Context->CSSetUnorderedAccessViews(static_cast<UINT>(Slot), 1, &View, nullptr);
        }

    protected:
        /**
         * @brief 용도별 바인딩 플래그와 부가 플래그를 채웁니다.
         * @param[in] Description 크기와 사용 방식이 채워진 버퍼 설명입니다.
         */
        void FillDescription(D3D11_BUFFER_DESC& Description) const override {
            FBase::FillDescription(Description);

            Description.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
        }

        /**
         * @brief 새 버퍼를 가리키는 뷰를 만듭니다.
         * @param[in] Device 뷰를 만들 장치입니다.
         * @param[in] Buffer 새로 만든 버퍼입니다.
         * @param[out] Error 뷰 생성 실패 원인입니다.
         * @return 성공하면 true이며, 실패하면 기존 뷰를 유지해야 합니다.
         */
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

        /**
         * @brief 파생 클래스가 소유한 뷰를 해제합니다.
         */
        void ReleaseViews() override {
            FBase::ReleaseViews();
            mUnorderedAccessView.Reset();
        }

    private:
        /**
         * @brief 읽기·쓰기 구조화 버퍼가 공유하는 기본 GPU 버퍼 타입입니다.
         */
        using FBase = RenderCore::TStructuredBuffer<T, RenderCore::EBufferUpdate::Default>;

        /**
         * @brief 버퍼 전체에 접근하는 순서 없는 접근 뷰입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> mUnorderedAccessView{};
    };

}
