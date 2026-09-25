#pragma once

#include <d3d11.h>
#include <wrl/client.h>

struct FGraphicsBufferDescription {
    Uint32 mByteSize{0};
    Uint32 mStride{0};

    D3D11_USAGE mUsage{D3D11_USAGE_DEFAULT};

    Uint32 mBindFlags{0};
    Uint32 mCpuAccessFlags{0};
    Uint32 mMiscFlags{0};
};

class FGraphicsBuffer {
public:
    FGraphicsBuffer() = default;
    ~FGraphicsBuffer() = default;

    FGraphicsBuffer(const FGraphicsBuffer&) = delete;
    FGraphicsBuffer& operator=(const FGraphicsBuffer&) = delete;

    FGraphicsBuffer(FGraphicsBuffer&&) noexcept = default;
    FGraphicsBuffer& operator=(FGraphicsBuffer&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device, const FGraphicsBufferDescription& Description, const void* InitialData = nullptr);

    bool Update(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize, Uint32 DestinationOffset = 0);

    bool WriteDiscard(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize);
    bool WriteNoOverwrite(ID3D11DeviceContext* Context, const void* Data, Uint32 InByteSize, Uint32 DestinationOffset);

    bool CopyFrom(ID3D11DeviceContext* Context, const FGraphicsBuffer& Source);
    bool CopyFrom(ID3D11DeviceContext* Context, Uint32 DestinationOffset, const FGraphicsBuffer& Source, Uint32 SourceOffset, Uint32 InByteSize);

    void Reset();

public:
    [[nodiscard]] ID3D11Buffer* GetBuffer() const;

    [[nodiscard]] Uint32 GetByteSize() const;

    [[nodiscard]] Uint32 GetStride() const;

    [[nodiscard]] Uint32 GetBindFlags() const;

    [[nodiscard]] D3D11_USAGE GetUsage() const;

    [[nodiscard]] const FGraphicsBufferDescription& GetDescription() const;

    [[nodiscard]] bool IsValid() const;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
    FGraphicsBufferDescription mDescription{};
};
