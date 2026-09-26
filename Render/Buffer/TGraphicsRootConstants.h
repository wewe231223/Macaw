#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>

enum class EGraphicsShaderStage : Uint32 {
    None = 0,

    Vertex = 1 << 0,
    Hull = 1 << 1,
    Domain = 1 << 2,
    Geometry = 1 << 3,
    Pixel = 1 << 4,
    Compute = 1 << 5,

    Graphics = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4),
    All = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5)
};

EGraphicsShaderStage operator|(EGraphicsShaderStage Left, EGraphicsShaderStage Right);

EGraphicsShaderStage operator&(EGraphicsShaderStage Left, EGraphicsShaderStage Right);

EGraphicsShaderStage& operator|=(EGraphicsShaderStage& Left, EGraphicsShaderStage Right);

bool HasGraphicsShaderStage(EGraphicsShaderStage Value, EGraphicsShaderStage Stage);

#define GRAPHICS_ROOT_32BIT_OFFSET(Type, Member) static_cast<Uint32>(offsetof(Type, Member) / sizeof(Uint32))

template <Uint32 N>
concept CGraphicsRootConstantCount = N > 0 && (N % 4) == 0;

template <Uint32 ConstantCount>
    requires CGraphicsRootConstantCount<ConstantCount>
class TGraphicsRootConstants {
private:
    static constexpr Uint32 DataByteSize{ConstantCount * sizeof(Uint32)};

    static_assert(DataByteSize <= D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16, "Graphics root constants exceed the Direct3D 11 constant buffer limit.");

public:
    TGraphicsRootConstants() = default;
    ~TGraphicsRootConstants() = default;

    TGraphicsRootConstants(const TGraphicsRootConstants&) = delete;
    TGraphicsRootConstants& operator=(const TGraphicsRootConstants&) = delete;

    TGraphicsRootConstants(TGraphicsRootConstants&&) noexcept = default;
    TGraphicsRootConstants& operator=(TGraphicsRootConstants&&) noexcept = default;

public:
    bool Initialize(ID3D11Device* Device);

    template <typename T> bool SetGraphicsRoot32BitConstant(const T& SrcData, Uint32 DestOffsetIn32BitValues);

    template <typename T> bool SetGraphicsRoot32BitConstants(const T& SrcData, Uint32 DestOffsetIn32BitValues = 0);

    bool Bind(ID3D11DeviceContext* Context, Uint32 Slot, EGraphicsShaderStage ShaderStages);

    bool Commit(ID3D11DeviceContext* Context);

    void Reset();

public:
    [[nodiscard]] ID3D11Buffer* GetBuffer() const;

    [[nodiscard]] static constexpr Uint32 GetConstantCount();

    [[nodiscard]] static constexpr Uint32 GetByteSize();

    [[nodiscard]] bool IsValid() const;

    [[nodiscard]] bool IsDirty() const;

private:
    Microsoft::WRL::ComPtr<ID3D11Buffer> mBuffer{};
    std::array<Uint32, ConstantCount> mConstants{};

    bool mBDirty{false};
};

/*
[32 .......... ]
*/

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> bool TGraphicsRootConstants<ConstantCount>::Initialize(ID3D11Device* Device) {
    if (!Device) {
        return false;
    }

    Reset();

    D3D11_BUFFER_DESC Description{};
    Description.ByteWidth = DataByteSize;
    Description.Usage = D3D11_USAGE_DYNAMIC;
    Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Description.MiscFlags = 0;
    Description.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA InitialData{};
    InitialData.pSysMem = mConstants.data();

    if (FAILED(Device->CreateBuffer(&Description, &InitialData, mBuffer.ReleaseAndGetAddressOf()))) {
        Reset();
        return false;
    }

    mBDirty = false;

    return true;
}

template <Uint32 ConstantCount>
    requires CGraphicsRootConstantCount<ConstantCount>
template <typename T> bool TGraphicsRootConstants<ConstantCount>::SetGraphicsRoot32BitConstant(const T& SrcData, Uint32 DestOffsetIn32BitValues) {
    static_assert(std::is_trivially_copyable_v<T>, "Graphics root constant data must be trivially copyable.");
    static_assert(sizeof(T) == sizeof(Uint32), "SetGraphicsRoot32BitConstant requires exactly one 32-bit value.");

    if (DestOffsetIn32BitValues >= ConstantCount) {
        return false;
    }

    std::memcpy(mConstants.data() + DestOffsetIn32BitValues, &SrcData, sizeof(T));

    mBDirty = true;

    return true;
}

template <Uint32 ConstantCount>
    requires CGraphicsRootConstantCount<ConstantCount>
template <typename T> bool TGraphicsRootConstants<ConstantCount>::SetGraphicsRoot32BitConstants(const T& SrcData, Uint32 DestOffsetIn32BitValues) {
    static_assert(std::is_trivially_copyable_v<T>, "Graphics root constant data must be trivially copyable.");
    static_assert(sizeof(T) % sizeof(Uint32) == 0, "Graphics root constant data size must be a multiple of 32 bits.");

    constexpr Uint32 Num32BitValues{static_cast<Uint32>(sizeof(T) / sizeof(Uint32))};

    if (DestOffsetIn32BitValues > ConstantCount || Num32BitValues > ConstantCount - DestOffsetIn32BitValues) {
        return false;
    }

    std::memcpy(mConstants.data() + DestOffsetIn32BitValues, &SrcData, sizeof(T));

    mBDirty = true;

    return true;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> bool TGraphicsRootConstants<ConstantCount>::Bind(ID3D11DeviceContext* Context, Uint32 Slot, EGraphicsShaderStage ShaderStages) {
    if (!Context || !mBuffer) {
        return false;
    }

    if (mBDirty && !Commit(Context)) {
        return false;
    }

    ID3D11Buffer* ConstantBuffer{mBuffer.Get()};

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Vertex)) {
        Context->VSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Hull)) {
        Context->HSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Domain)) {
        Context->DSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Geometry)) {
        Context->GSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Pixel)) {
        Context->PSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    if (HasGraphicsShaderStage(ShaderStages, EGraphicsShaderStage::Compute)) {
        Context->CSSetConstantBuffers(Slot, 1, &ConstantBuffer);
    }

    return true;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> bool TGraphicsRootConstants<ConstantCount>::Commit(ID3D11DeviceContext* Context) {
    if (!Context || !mBuffer) {
        return false;
    }

    if (!mBDirty) {
        return true;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource{};

    if (FAILED(Context->Map(mBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource))) {
        return false;
    }

    std::memcpy(MappedResource.pData, mConstants.data(), DataByteSize);

    Context->Unmap(mBuffer.Get(), 0);

    mBDirty = false;

    return true;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> void TGraphicsRootConstants<ConstantCount>::Reset() {
    mBuffer.Reset();
    mConstants.fill(0);

    mBDirty = false;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> [[nodiscard]] ID3D11Buffer* TGraphicsRootConstants<ConstantCount>::GetBuffer() const {
    return mBuffer.Get();
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> [[nodiscard]] constexpr Uint32 TGraphicsRootConstants<ConstantCount>::GetConstantCount() {
    return ConstantCount;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> [[nodiscard]] constexpr Uint32 TGraphicsRootConstants<ConstantCount>::GetByteSize() {
    return DataByteSize;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> [[nodiscard]] bool TGraphicsRootConstants<ConstantCount>::IsValid() const {
    return mBuffer != nullptr;
}

template <Uint32 ConstantCount> requires CGraphicsRootConstantCount<ConstantCount> [[nodiscard]] bool TGraphicsRootConstants<ConstantCount>::IsDirty() const {
    return mBDirty;
}
