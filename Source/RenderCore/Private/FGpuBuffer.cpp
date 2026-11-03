#include "RenderCore/FGpuBuffer.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <utility>

namespace RenderCore {

    RenderCore::FGpuBuffer::FGpuBuffer(Core::Uint32 Stride, RenderCore::EBufferUpdate Update)
        : mStride{Stride},
          mUpdate{Update} {
    }

    RenderCore::FGpuBuffer::FGpuBuffer(RenderCore::FGpuBuffer&& Other) noexcept
        : mBuffer{std::move(Other.mBuffer)},
          mStride{Other.mStride},
          mCapacity{std::exchange(Other.mCapacity, 0u)},
          mUpdate{Other.mUpdate} {
    }

    RenderCore::FGpuBuffer& RenderCore::FGpuBuffer::operator=(RenderCore::FGpuBuffer&& Other) noexcept {
        mBuffer = std::move(Other.mBuffer);
        mStride = Other.mStride;
        mCapacity = std::exchange(Other.mCapacity, 0u);
        mUpdate = Other.mUpdate;
        return *this;
    }

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

    void RenderCore::FGpuBuffer::Reset() {
        ReleaseViews();
        mBuffer.Reset();
        mCapacity = 0;
    }

    bool RenderCore::FGpuBuffer::IsCreated() const {
        return mBuffer != nullptr;
    }

    bool RenderCore::FGpuBuffer::IsCreatedOn(ID3D11Device* Device) const {
        if (mBuffer == nullptr) {
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};
        mBuffer->GetDevice(ResourceDevice.GetAddressOf());
        return ResourceDevice.Get() == Device;
    }

    std::size_t RenderCore::FGpuBuffer::GetResourceBytes() const {
        return static_cast<std::size_t>(mCapacity) * mStride;
    }

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

    ID3D11Buffer* RenderCore::FGpuBuffer::GetBuffer() const {
        return mBuffer.Get();
    }

    Core::Uint32 RenderCore::FGpuBuffer::GetStride() const {
        return mStride;
    }

    Core::Uint64 RenderCore::FGpuBuffer::GetMaximumCount() const {
        return std::numeric_limits<Core::Uint32>::max() / mStride;
    }

    Core::Uint64 RenderCore::FGpuBuffer::GetGrowthCapacity(Core::Uint64 Count) const {
        return std::bit_ceil(Count);
    }

    bool RenderCore::FGpuBuffer::CreateViews(ID3D11Device*, ID3D11Buffer*, Core::FString&) {
        return true;
    }

    void RenderCore::FGpuBuffer::ReleaseViews() {
    }

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
