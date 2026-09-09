#pragma once

#include <d3d11.h>

#include "Core/Asset/FAssetRegistry.h"

class FWorldContext {
  public:
	void SetAssetRegistry(FAssetRegistry *InAssetRegistry);
	FAssetRegistry *GetAssetRegistry() const noexcept;

	void SetDevice(ID3D11Device *InDevice);
	ID3D11Device *GetDevice() const noexcept;

  private:
	FAssetRegistry *AssetRegistry = nullptr;
	ID3D11Device *Device = nullptr;
};
