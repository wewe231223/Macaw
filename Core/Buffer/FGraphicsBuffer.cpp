#include "pch.h"
#include "FGraphicsBuffer.h"

#include <cstring>

bool FGraphicsBuffer::Initialize(ID3D11Device* Device, const FGraphicsBufferDescription& InDescription, const void* InitialData) {
    if (!Device || InDescription.mByteSize == 0) {
        return false;
    }

    D3D11_BUFFER_DESC BufferDesc{};
    BufferDesc.ByteWidth = InDescription.mByteSize;
    BufferDesc.Usage = InDescription.mUsage;
    BufferDesc.BindFlags = InDescription.mBindFlags;
    BufferDesc.CPUAccessFlags = InDescription.mCpuAccessFlags;
    BufferDesc.MiscFlags = InDescription.mMiscFlags;
    BufferDesc.StructureByteStride = InDescription.mStride;

    D3D11_SUBRESOURCE_DATA SubresourceData{};
    SubresourceData.pSysMem = InitialData;

    Microsoft::WRL::ComPtr<ID3D11Buffer> NewBuffer{};

    const HRESULT Result{Device->CreateBuffer(&BufferDesc, InitialData ? &SubresourceData : nullptr, NewBuffer.GetAddressOf())};
    if (FAILED(Result)) {
        return false;
    }

    mBuffer = std::move(NewBuffer);
    mDescription = InDescription;

    return true;
}

bool FGraphicsBuffer::Update(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize, Uint32 DestinationOffset) {
    if (!Context || !mBuffer || !Data || InByteSize == 0) {
        return false;
    }

    if (mDescription.mUsage != D3D11_USAGE_DEFAULT) {
        return false;
    }

    if (DestinationOffset > mDescription.mByteSize || InByteSize > mDescription.mByteSize - DestinationOffset) {
        return false;
    }

    if (DestinationOffset == 0 && InByteSize == mDescription.mByteSize) {
        Context->UpdateSubresource(mBuffer.Get(), 0, nullptr, Data, 0, 0);
        return true;
    }

    D3D11_BOX Box{};
    Box.left = DestinationOffset;
    Box.right = DestinationOffset + InByteSize;
    Box.top = 0;
    Box.bottom = 1;
    Box.front = 0;
    Box.back = 1;

    Context->UpdateSubresource(mBuffer.Get(), 0, &Box, Data, 0, 0);

    return true;
}

bool FGraphicsBuffer::WriteDiscard(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize) {
    if (!Context || !mBuffer || !Data || InByteSize == 0) {
        return false;
    }

    if (mDescription.mUsage != D3D11_USAGE_DYNAMIC || !(mDescription.mCpuAccessFlags & D3D11_CPU_ACCESS_WRITE)) {
        return false;
    }

    if (InByteSize > mDescription.mByteSize) {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource{};

    const HRESULT Result{Context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource)};
    if (FAILED(Result)) {
        return false;
    }

    std::memcpy(MappedResource.pData, Data, InByteSize);

    Context->Unmap(mBuffer.Get(), 0);

    return true;
}

bool FGraphicsBuffer::WriteNoOverwrite(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize, Uint32 DestinationOffset) {
    if (!Context || !mBuffer || !Data || InByteSize == 0) {
        return false;
    }

    if (mDescription.mUsage != D3D11_USAGE_DYNAMIC || !(mDescription.mCpuAccessFlags & D3D11_CPU_ACCESS_WRITE)) {
        return false;
    }

    if (DestinationOffset > mDescription.mByteSize || InByteSize > mDescription.mByteSize - DestinationOffset) {
        return false;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource{};

    const HRESULT Result{Context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &MappedResource)};
    if (FAILED(Result)) {
        return false;
    }

    std::memcpy(static_cast<Uint8*>(MappedResource.pData) + DestinationOffset, Data, InByteSize);

    Context->Unmap(mBuffer.Get(), 0);

    return true;
}

bool FGraphicsBuffer::CopyFrom(ID3D11DeviceContext* Context, const FGraphicsBuffer& Source) {
    if (!Context || !mBuffer || !Source.mBuffer) {
        return false;
    }

    if (mDescription.mByteSize != Source.mDescription.mByteSize) {
        return false;
    }

    Context->CopyResource(mBuffer.Get(), Source.mBuffer.Get());

    return true;
}

bool FGraphicsBuffer::CopyFrom(ID3D11DeviceContext* Context, Uint32 DestinationOffset, const FGraphicsBuffer& Source, Uint32 SourceOffset, Uint32 InByteSize) {
    if (!Context || !mBuffer || !Source.mBuffer || InByteSize == 0) {
        return false;
    }

    if (DestinationOffset > mDescription.mByteSize || InByteSize > mDescription.mByteSize - DestinationOffset) {
        return false;
    }

    if (SourceOffset > Source.mDescription.mByteSize || InByteSize > Source.mDescription.mByteSize - SourceOffset) {
        return false;
    }

    D3D11_BOX SourceBox{};
    SourceBox.left = SourceOffset;
    SourceBox.right = SourceOffset + InByteSize;
    SourceBox.top = 0;
    SourceBox.bottom = 1;
    SourceBox.front = 0;
    SourceBox.back = 1;

    Context->CopySubresourceRegion(mBuffer.Get(), 0, DestinationOffset, 0, 0, Source.mBuffer.Get(), 0, &SourceBox);

    return true;
}

void FGraphicsBuffer::Reset() {
    mBuffer.Reset();
    mDescription = {};
}

[[nodiscard]] ID3D11Buffer* FGraphicsBuffer::GetBuffer() const {
    return mBuffer.Get();
}

[[nodiscard]] Uint32 FGraphicsBuffer::GetByteSize() const {
    return mDescription.mByteSize;
}

[[nodiscard]] Uint32 FGraphicsBuffer::GetStride() const {
    return mDescription.mStride;
}

[[nodiscard]] Uint32 FGraphicsBuffer::GetBindFlags() const {
    return mDescription.mBindFlags;
}

[[nodiscard]] D3D11_USAGE FGraphicsBuffer::GetUsage() const {
    return mDescription.mUsage;
}

[[nodiscard]] const FGraphicsBufferDescription& FGraphicsBuffer::GetDescription() const {
    return mDescription;
}

[[nodiscard]] bool FGraphicsBuffer::IsValid() const {
    return mBuffer != nullptr;
}
