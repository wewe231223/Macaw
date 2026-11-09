/**
 * @file FGpuBuffer.h
 * @brief 모든 Direct3D 11 버퍼가 공유하는 생성, 용량 관리와 업로드 절차를 선언합니다.
 * @ingroup RenderCore
 */

#pragma once

#include "Core/Types.h"
#include "RenderCore/EBufferUpdate.h"

#include <d3d11.h>
#include <wrl/client.h>
#include <cstddef>
#include <span>

namespace RenderCore {

    /**
     * @brief GPU 버퍼의 추상 기반 클래스입니다.
     * @details 할당 절차, 용량 관리와 갱신 방식은 이 클래스가 맡고, 용도별 설정과 뷰는 파생 클래스가 재정의합니다.
     *          GPU 자원은 밖으로 노출하지 않으며 바인딩은 파생 클래스의 Bind로만 수행합니다.
     */
    class FGpuBuffer {
    public:
        /**
         * @brief 소유한 GPU 자원을 해제합니다.
         */
        virtual ~FGpuBuffer() = default;

        /**
         * @brief GPU 자원 소유 상태의 복사를 금지합니다.
         */
        FGpuBuffer(const RenderCore::FGpuBuffer&) = delete;

        /**
         * @brief GPU 자원 소유 상태의 복사 대입을 금지합니다.
         */
        RenderCore::FGpuBuffer& operator=(const RenderCore::FGpuBuffer&) = delete;

    public:
        /**
         * @brief 원소 수를 담을 용량과 뷰를 확보합니다.
         * @param[in] Device 자원을 만들 장치입니다.
         * @param[in] Count 필요한 원소 수입니다.
         * @param[out] Error 원소 수 오류 또는 할당 실패 원인입니다.
         * @return 용량이 충분하거나 할당에 성공하면 true입니다.
         * @note 재할당하면 기존 내용은 보존하지 않습니다. Immutable 버퍼는 초기 데이터가 필요하므로 실패합니다.
         */
        bool Reserve(ID3D11Device* Device, Core::Uint64 Count, Core::FString& Error);

        /**
         * @brief 소유한 GPU 자원과 용량 정보를 초기화합니다.
         */
        void Reset();

        /**
         * @brief GPU 자원을 할당했는지 확인합니다.
         * @return 할당한 버퍼가 있으면 true입니다.
         */
        bool IsCreated() const;

        /**
         * @brief 지정한 장치에 GPU 자원을 할당했는지 확인합니다.
         * @param[in] Device 비교할 장치입니다.
         * @return 할당한 버퍼가 그 장치의 것이면 true입니다.
         */
        bool IsCreatedOn(ID3D11Device* Device) const;

        /**
         * @brief 현재 확보한 GPU 버퍼 크기를 반환합니다.
         * @return 원소 용량에 원소 크기를 곱한 바이트 수입니다.
         */
        std::size_t GetResourceBytes() const;

    protected:
        /**
         * @brief GPU 자원이 없는 빈 버퍼를 생성합니다.
         * @param[in] Stride 원소 하나의 바이트 수입니다.
         * @param[in] Update CPU 데이터를 기록하는 방식입니다.
         */
        FGpuBuffer(Core::Uint32 Stride, RenderCore::EBufferUpdate Update);

        /**
         * @brief GPU 자원 소유권을 옮기고 원본을 미할당 상태로 만듭니다.
         * @param[in,out] Other 자원을 넘겨줄 버퍼입니다.
         */
        FGpuBuffer(RenderCore::FGpuBuffer&& Other) noexcept;

        /**
         * @brief GPU 자원 소유권을 옮겨 대입하고 원본을 미할당 상태로 만듭니다.
         * @param[in,out] Other 자원을 넘겨줄 버퍼입니다.
         * @return 자원을 넘겨받은 이 버퍼입니다.
         */
        RenderCore::FGpuBuffer& operator=(RenderCore::FGpuBuffer&& Other) noexcept;

        /**
         * @brief 필요한 경우 버퍼를 재할당하고 갱신 방식에 맞게 바이트를 기록합니다.
         * @param[in] Context 업로드할 장치 컨텍스트입니다.
         * @param[in] Data 비어 있지 않은 연속된 원소 바이트입니다.
         * @param[out] Error 데이터 오류, 할당 또는 기록 실패 원인입니다.
         * @return 업로드에 성공하면 true입니다.
         */
        bool UploadBytes(ID3D11DeviceContext* Context, std::span<const std::byte> Data, Core::FString& Error);

        /**
         * @brief 데이터 크기에 꼭 맞는 버퍼를 초기 데이터와 함께 만듭니다.
         * @param[in] Device 자원을 만들 장치입니다.
         * @param[in] Data 비어 있지 않은 연속된 원소 바이트입니다.
         * @param[out] Error 데이터 오류 또는 할당 실패 원인입니다.
         * @return 같은 장치에 버퍼가 이미 있거나 생성에 성공하면 true입니다.
         */
        bool CreateFromBytes(ID3D11Device* Device, std::span<const std::byte> Data, Core::FString& Error);

        /**
         * @brief 파생 클래스의 바인딩에 사용할 버퍼의 비소유 주소를 반환합니다.
         * @return 할당한 버퍼이며 미할당 상태이면 nullptr입니다.
         */
        ID3D11Buffer* GetBuffer() const;

        /**
         * @brief 원소 하나의 바이트 수를 반환합니다.
         * @return 원소 크기입니다.
         */
        Core::Uint32 GetStride() const;

    protected:
        /**
         * @brief 용도별 바인딩 플래그와 부가 플래그를 채웁니다.
         * @param[in,out] Description 크기와 사용 방식이 채워진 버퍼 설명입니다.
         */
        virtual void FillDescription(D3D11_BUFFER_DESC& Description) const = 0;

        /**
         * @brief 이 버퍼가 담을 수 있는 최대 원소 수를 반환합니다.
         * @return 기본 구현은 바이트 크기가 32비트에 들어가는 원소 수입니다.
         */
        virtual Core::Uint64 GetMaximumCount() const;

        /**
         * @brief 재할당할 때 확보할 원소 수를 정합니다.
         * @param[in] Count 필요한 원소 수입니다.
         * @return 기본 구현은 Count 이상의 가장 작은 2의 거듭제곱입니다.
         */
        virtual Core::Uint64 GetGrowthCapacity(Core::Uint64 Count) const;

        /**
         * @brief 새 버퍼를 가리키는 뷰를 만듭니다.
         * @param[in] Device 뷰를 만들 장치입니다.
         * @param[in] Buffer 새로 만든 버퍼입니다.
         * @param[out] Error 뷰 생성 실패 원인입니다.
         * @return 성공하면 true이며, 실패하면 기존 뷰를 유지해야 합니다.
         */
        virtual bool CreateViews(ID3D11Device* Device, ID3D11Buffer* Buffer, Core::FString& Error);

        /**
         * @brief 파생 클래스가 소유한 뷰를 해제합니다.
         */
        virtual void ReleaseViews();

    private:
        /**
         * @brief 버퍼와 뷰를 만든 뒤 기존 자원과 교체합니다.
         * @param[in] Device 자원을 만들 장치입니다.
         * @param[in] Capacity 확보할 원소 수입니다.
         * @param[in] InitialData 초기 데이터이며 없으면 nullptr입니다.
         * @param[out] Error 할당 실패 원인입니다.
         * @return 성공하면 true이며, 실패하면 기존 자원을 유지합니다.
         */
        bool CreateResource(ID3D11Device* Device, Core::Uint32 Capacity, const void* InitialData, Core::FString& Error);

    private:
        /**
         * @brief GPU 버퍼입니다.
         */
        Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};

        /**
         * @brief 원소 하나의 바이트 수입니다.
         */
        Core::Uint32 mStride{};

        /**
         * @brief 버퍼에 확보한 원소 수입니다.
         */
        Core::Uint32 mCapacity{};

        /**
         * @brief CPU 데이터를 기록하는 방식입니다.
         */
        RenderCore::EBufferUpdate mUpdate{RenderCore::EBufferUpdate::Default};
    };

}
