#include "PCH.h"
#include "FWorldContext.h"

void FWorldContext::SetAssetRegistry(FAssetRegistry *InAssetRegistry) {
	AssetRegistry = InAssetRegistry;
}

FAssetRegistry *FWorldContext::GetAssetRegistry() const noexcept {
	return AssetRegistry;
}

void FWorldContext::SetDevice(ID3D11Device *InDevice) {
	Device = InDevice;
}

ID3D11Device *FWorldContext::GetDevice() const noexcept {
	return Device;
}
