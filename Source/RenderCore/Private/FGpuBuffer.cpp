/**
 * @file FGpuBuffer.cpp
 * @brief 모든 Direct3D 11 버퍼가 공유하는 생성, 용량 관리와 업로드 절차를 선언합니다.
 * @ingroup RenderCore
 */

#include "RenderCore/FGpuBuffer.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <utility>

namespace RenderCore {

    /**
     * @brief GPU 자원이 없는 빈 버퍼를 생성합니다.
     */
    RenderCore::FGpuBuffer::FGpuBuffer(Core::Uint32 Stride, RenderCore::EBufferUpdate Update)
        : mStride{Stride},
          mUpdate{Update} {
    }

    /**
     * @brief GPU 자원 소유권을 옮기고 원본을 미할당 상태로 만듭니다.
     */
    RenderCore::FGpuBuffer::FGpuBuffer(RenderCore::FGpuBuffer&& Other) noexcept
        : mBuffer{std::move(Other.mBuffer)},
          mStride{Other.mStride},
          mCapacity{std::exchange(Other.mCapacity, 0u)},
          mUpdate{Other.mUpdate} {
    }

    /**
     * @brief GPU 자원 소유권을 옮겨 대입하고 원본을 미할당 상태로 만듭니다.
     */
    RenderCore::FGpuBuffer& RenderCore::FGpuBuffer::operator=(RenderCore::FGpuBuffer&& Other) noexcept {
        mBuffer = std::move(Other.mBuffer);
        mStride = Other.mStride;
        mCapacity = std::exchange(Other.mCapacity, 0u);
        mUpdate = Other.mUpdate;

        return *this;
    }

    /**
     * @brief 원소 수를 담을 용량과 뷰를 확보합니다.
     */
    bool RenderCore::FGpuBuffer::Reserve(ID3D11Device* Device, Core::Uint64 Count, Core::FString& Error) {
        Error.clear();

        if (Device == nullptr || Count == 0 || Count > GetMaximumCount()) {
            Error = "Invalid GPU buffer device or element count";

            return false;
        }

        if (Count <= mCapacity && IsCreatedOn(Device)) {
            return true;
        }

        if (mUpdate == RenderCore::EBufferUpdate::Immutable) {
            Error = "An immutable GPU buffer needs initial data";

            return false;
        }

        return CreateResource(Device, static_cast<Core::Uint32>(std::min(GetGrowthCapacity(Count), GetMaximumCount())), nullptr, Error);
    }

    /**
     * @brief 소유한 GPU 자원과 용량 정보를 초기화합니다.
     */
    void RenderCore::FGpuBuffer::Reset() {
        ReleaseViews();
        mBuffer.Reset();
        mCapacity = 0;
    }

    /**
     * @brief GPU 자원을 할당했는지 확인합니다.
     */
    bool RenderCore::FGpuBuffer::IsCreated() const {
        return mBuffer != nullptr;
    }

    /**
     * @brief 지정한 장치에 GPU 자원을 할당했는지 확인합니다.
     */
    bool RenderCore::FGpuBuffer::IsCreatedOn(ID3D11Device* Device) const {
        if (mBuffer == nullptr) {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};

        mBuffer->GetDevice(ResourceDevice.GetAddressOf());

        return ResourceDevice.Get() == Device;
    }

    /**
     * @brief 현재 확보한 GPU 버퍼 크기를 반환합니다.
     */
    std::size_t RenderCore::FGpuBuffer::GetResourceBytes() const {
        return static_cast<std::size_t>(mCapacity) * mStride;
    }

    /**
     * @brief 필요한 경우 버퍼를 재할당하고 갱신 방식에 맞게 바이트를 기록합니다.
     */
    bool RenderCore::FGpuBuffer::UploadBytes(ID3D11DeviceContext* Context, std::span<const std::byte> Data, Core::FString& Error) {
        Error.clear();

        if (Context == nullptr || Data.empty() || Data.size() % mStride != 0) {
            Error = "Invalid GPU buffer data";

            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> Device{};

        Context->GetDevice(Device.GetAddressOf());

        if (!Reserve(Device.Get(), Data.size() / mStride, Error)) {
            return false;
        }

        if (mUpdate == RenderCore::EBufferUpdate::Immutable) {
            Error = "An immutable GPU buffer cannot be updated";

            return false;
        }

        if (mUpdate == RenderCore::EBufferUpdate::Dynamic) {
            D3D11_MAPPED_SUBRESOURCE Mapped{};

            if (FAILED(Context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) {
                Error = "GPU buffer mapping failed";

                return false;
            }

            std::memcpy(Mapped.pData, Data.data(), Data.size());
            Context->Unmap(mBuffer.Get(), 0);

            return true;
        }

        const D3D11_BOX Box{0, 0, 0, static_cast<UINT>(Data.size()), 1, 1};

        Context->UpdateSubresource(mBuffer.Get(), 0, &Box, Data.data(), 0, 0);

        return true;
    }

    /**
     * @brief 데이터 크기에 꼭 맞는 버퍼를 초기 데이터와 함께 만듭니다.
     */
    bool RenderCore::FGpuBuffer::CreateFromBytes(ID3D11Device* Device, std::span<const std::byte> Data, Core::FString& Error) {
        Error.clear();

        if (Device == nullptr || Data.empty() || Data.size() % mStride != 0 || Data.size() / mStride > GetMaximumCount()) {
            Error = "Invalid GPU buffer device or initial data";

            return false;
        }

        if (IsCreatedOn(Device)) {
            return true;
        }

        return CreateResource(Device, static_cast<Core::Uint32>(Data.size() / mStride), Data.data(), Error);
    }

    /**
     * @brief 파생 클래스의 바인딩에 사용할 버퍼의 비소유 주소를 반환합니다.
     */
    ID3D11Buffer* RenderCore::FGpuBuffer::GetBuffer() const {
        return mBuffer.Get();
    }

    /**
     * @brief 원소 하나의 바이트 수를 반환합니다.
     */
    Core::Uint32 RenderCore::FGpuBuffer::GetStride() const {
        return mStride;
    }

    /**
     * @brief 이 버퍼가 담을 수 있는 최대 원소 수를 반환합니다.
     */
    Core::Uint64 RenderCore::FGpuBuffer::GetMaximumCount() const {
        return std::numeric_limits<Core::Uint32>::max() / mStride;
    }

    /**
     * @brief 재할당할 때 확보할 원소 수를 정합니다.
     */
    Core::Uint64 RenderCore::FGpuBuffer::GetGrowthCapacity(Core::Uint64 Count) const {
        return std::bit_ceil(Count);
    }

    /**
     * @brief 새 버퍼를 가리키는 뷰를 만듭니다.
     */
    bool RenderCore::FGpuBuffer::CreateViews(ID3D11Device*, ID3D11Buffer*, Core::FString&) {
        return true;
    }

    /**
     * @brief 파생 클래스가 소유한 뷰를 해제합니다.
     */
    void RenderCore::FGpuBuffer::ReleaseViews() {
    }

    /**
     * @brief 버퍼와 뷰를 만든 뒤 기존 자원과 교체합니다.
     */
    bool RenderCore::FGpuBuffer::CreateResource(ID3D11Device* Device, Core::Uint32 Capacity, const void* InitialData, Core::FString& Error) {
        D3D11_BUFFER_DESC Description{};
        Description.ByteWidth = Capacity * mStride;

        switch (mUpdate) {
            case RenderCore::EBufferUpdate::Default:
                Description.Usage = D3D11_USAGE_DEFAULT;

                break;
            case RenderCore::EBufferUpdate::Dynamic:
                Description.Usage = D3D11_USAGE_DYNAMIC;
                Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

                break;
            case RenderCore::EBufferUpdate::Immutable:
                Description.Usage = D3D11_USAGE_IMMUTABLE;

                break;
        }

        FillDescription(Description);

        D3D11_SUBRESOURCE_DATA Initial{};
        Initial.pSysMem = InitialData;

        Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};

        if (FAILED(Device->CreateBuffer(&Description, InitialData != nullptr ? &Initial : nullptr, Buffer.GetAddressOf()))) {
            Error = "GPU buffer allocation failed";

            return false;
        }

        if (!CreateViews(Device, Buffer.Get(), Error)) {
            return false;
        }

        mBuffer = std::move(Buffer);
        mCapacity = Capacity;

        return true;
    }

}
