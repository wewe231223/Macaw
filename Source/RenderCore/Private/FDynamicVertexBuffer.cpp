/**
 * @file FDynamicVertexBuffer.cpp
 * @brief 동적 정점 버퍼의 범위 검증, 재할당과 매핑 업로드를 구현합니다.
 * @ingroup RenderCore
 */

#include "RenderCore/FDynamicVertexBuffer.h"

#include <cstring>
#include <limits>
#include <utility>

namespace RenderCore {

    bool RenderCore::FDynamicVertexBuffer::Upload(ID3D11DeviceContext* Context, std::span<const std::byte> Data, Core::Uint32 Stride, Core::Uint32 Capacity, Core::FString& Error) {
        Error.clear();

        if (Context == nullptr || Stride == 0 || Data.empty() || Data.size() % Stride != 0 || Data.size() / Stride > Capacity || Capacity > std::numeric_limits<Core::Uint32>::max() / Stride) {
            Error = "Invalid dynamic vertex buffer data or capacity";
            return false;
        }

        Microsoft::WRL::ComPtr<ID3D11Device> Device{};
        Context->GetDevice(Device.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Device> ResourceDevice{};

        if (mBuffer != nullptr) {
            mBuffer->GetDevice(ResourceDevice.GetAddressOf());
        }

        if (mBuffer == nullptr || Data.size() / Stride > mCapacity || Stride != mStride || ResourceDevice.Get() != Device.Get()) {
            D3D11_BUFFER_DESC Description{};
            Description.ByteWidth = Capacity * Stride;
            Description.Usage = D3D11_USAGE_DYNAMIC;
            Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};

            if (FAILED(Device->CreateBuffer(&Description, nullptr, Buffer.GetAddressOf()))) {
                Error = "Dynamic vertex buffer allocation failed";
                return false;
            }

            mBuffer = std::move(Buffer);
            mCapacity = Capacity;
            mStride = Stride;
        }

        D3D11_MAPPED_SUBRESOURCE Mapped{};

        if (FAILED(Context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped))) {
            Error = "Dynamic vertex buffer mapping failed";
            return false;
        }

        std::memcpy(Mapped.pData, Data.data(), Data.size());
        Context->Unmap(mBuffer.Get(), 0);

        return true;
    }

    void RenderCore::FDynamicVertexBuffer::Reset() {
        mBuffer.Reset();
        mCapacity = 0;
        mStride = 0;
    }

    ID3D11Buffer* RenderCore::FDynamicVertexBuffer::GetBuffer() const {
        return mBuffer.Get();
    }

    std::size_t RenderCore::FDynamicVertexBuffer::GetResourceBytes() const {
        return static_cast<std::size_t>(mCapacity) * mStride;
    }

}
