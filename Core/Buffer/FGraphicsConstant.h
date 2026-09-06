#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <type_traits>

enum class EGraphicsShaderStage : uint8 {
	None = 0,
	Vertex = 1 << 0,
	Hull = 1 << 1,
	Domain = 1 << 2,
	Geometry = 1 << 3,
	Pixel = 1 << 4,
	Compute = 1 << 5,

	Graphics = Vertex | Hull | Domain | Geometry | Pixel,
	All = Graphics | Compute
};

constexpr EGraphicsShaderStage operator|(EGraphicsShaderStage Left, EGraphicsShaderStage Right) {
	return static_cast<EGraphicsShaderStage>(static_cast<uint8>(Left) | static_cast<uint8>(Right));
}

constexpr EGraphicsShaderStage operator&(EGraphicsShaderStage Left, EGraphicsShaderStage Right) {
	return static_cast<EGraphicsShaderStage>(static_cast<uint8>(Left) & static_cast<uint8>(Right));
}

constexpr bool HasShaderStage(EGraphicsShaderStage Value, EGraphicsShaderStage Stage) {
	return static_cast<uint8>(Value & Stage) != 0;
}

class FGraphicsConstant {
public:
	FGraphicsConstant() = default;
	~FGraphicsConstant() = default;

	FGraphicsConstant(const FGraphicsConstant&) = delete;
	FGraphicsConstant& operator=(const FGraphicsConstant&) = delete;

	FGraphicsConstant(FGraphicsConstant&&) noexcept = default;
	FGraphicsConstant& operator=(FGraphicsConstant&&) noexcept = default;

public:
	bool Initialize(ID3D11Device* Device, uint32 ByteSize);

	template<typename T>
	bool Initialize(ID3D11Device* Device) {
		static_assert(std::is_trivially_copyable_v<T>, "FGraphicsConstant requires trivially copyable data.");

		return Initialize(Device, sizeof(T));
	}

	bool Update(ID3D11DeviceContext* Context, const void* Data, uint32 ByteSize);

	template<typename T>
	bool Update(ID3D11DeviceContext* Context, const T& Data) {
		static_assert(std::is_trivially_copyable_v<T>, "FGraphicsConstant requires trivially copyable data.");

		return Update(Context, &Data, sizeof(T));
	}

	void Bind(ID3D11DeviceContext* Context, uint32 Slot, EGraphicsShaderStage ShaderStages) const;

	void Reset();

public:
	[[nodiscard]] ID3D11Buffer* GetBuffer() const { return Buffer.Get(); }

	[[nodiscard]] uint32 GetByteSize() const { return ByteSize; }
	[[nodiscard]] uint32 GetBufferByteSize() const { return BufferByteSize; }

	[[nodiscard]] bool IsValid() const { return Buffer != nullptr; }

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer{};

	uint32 ByteSize{};
	uint32 BufferByteSize = 0;
};