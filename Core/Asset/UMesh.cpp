#include "PCH.h"
#include "UMesh.h"

#include "../../ErrorHandler.h"

ID3D11Buffer* UMesh::GetVertexBuffer(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount()) {
		return nullptr;
	}

	return VertexBuffers[Index].Get();
}

ID3D11Buffer* UMesh::GetIndexBuffer() const {
	return IndexBuffer.Get();
}

bool UMesh::HasVertexAttribute(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount()) {
		return false;
	}

	return AttributeStorage[Index] != nullptr;
}

uint32 UMesh::GetVertexCount() const {
	return VertexCount;
}

uint32 UMesh::GetIndexCount() const {
	return IndexCount;
}

uint32 UMesh::GetVertexStride(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return 0;
	}

	return AttributeStorage[Index]->GetStride();
}

uint32 UMesh::GetVertexAttributeCount(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return 0;
	}

	return AttributeStorage[Index]->GetCount();
}

const void* UMesh::GetVertexData(EVertexAttribute Attribute) const {
	const size_t Index = GetAttributeIndex(Attribute);

	if (Index >= GetAttributeCount() || !AttributeStorage[Index]) {
		return nullptr;
	}

	return AttributeStorage[Index]->GetData();
}

bool UMesh::CreateIndexBuffer(ID3D11Device* Device, const TArray<uint32>& InIndices) {
	if (Device == nullptr || InIndices.empty()) {
		return false;
	}

	const size_t ByteSize = InIndices.size() * sizeof(uint32);

	if (ByteSize > std::numeric_limits<UINT>::max()) {
		return false;
	}

	D3D11_BUFFER_DESC BufferDesc{};
	BufferDesc.ByteWidth = static_cast<UINT>(ByteSize);
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA InitialData{};
	InitialData.pSysMem = InIndices.data();

	Microsoft::WRL::ComPtr<ID3D11Buffer> Buffer;

	const HRESULT Result = Device->CreateBuffer(&BufferDesc, &InitialData, Buffer.GetAddressOf());

	if (FAILED(Result)) {
		return false;
	}

	IndexBuffer = std::move(Buffer);

	Indices = InIndices;
	IndexCount = static_cast<uint32>(Indices.size());

	return true;
}

void UMesh::Reset() {
	for (Microsoft::WRL::ComPtr<ID3D11Buffer>& Buffer : VertexBuffers) {
		Buffer.Reset();
	}

	for (std::unique_ptr<FVertexAttributeStorageBase>& Storage : AttributeStorage) {
		Storage.reset();
	}

	IndexBuffer.Reset();
	Indices.clear();

	VertexCount = 0;
	IndexCount = 0;

	BoundsCenter = {};
	BoundsExtent = {};
}

const FVector3& UMesh::GetBoundsCenter() const
{
	return BoundsCenter;
}

const FVector3& UMesh::GetBoundsExtent() const
{
	return BoundsExtent;
}

void UMesh::CalculateBounds()
{
	const auto Positions = GetVertexAttributeData<EVertexAttribute::Position>();

	if (Positions.empty())
	{
		BoundsCenter = {};
		BoundsExtent = {};
		return;
	}

	FVector3 Min = Positions[0];
	FVector3 Max = Positions[0];

	for (const FVector3& Position : Positions)
	{
		Min.x = std::min(Min.x, Position.x);
		Min.y = std::min(Min.y, Position.y);
		Min.z = std::min(Min.z, Position.z);

		Max.x = std::max(Max.x, Position.x);
		Max.y = std::max(Max.y, Position.y);
		Max.z = std::max(Max.z, Position.z);
	}

	BoundsCenter = (Min + Max) * 0.5f;
	BoundsExtent = (Max - Min) * 0.5f;

	constexpr float MinExtent = 0.01f;

	BoundsExtent.x = std::max(BoundsExtent.x, MinExtent);
	BoundsExtent.y = std::max(BoundsExtent.y, MinExtent);
	BoundsExtent.z = std::max(BoundsExtent.z, MinExtent);
}