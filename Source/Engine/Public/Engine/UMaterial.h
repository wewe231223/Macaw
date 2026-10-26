#pragma once

#include "CoreUObject/UObject.h"
#include "Core/Math/FVector4.h"

#include <d3d11.h>
#include <wrl/client.h>

class FAssetManager;
class UTexture2D;

class UMaterial final : public UObject {
public:
    ReflectTypes(UMaterial, UObject);

public:
    UMaterial();
    ~UMaterial() override;

public:
    ReflectClass(UMaterial);
    const FString& GetShaderPath() const;
    bool SetShaderPath(const FString& Path);
    const FVector4& GetBaseColor() const;
    bool SetBaseColor(const FVector4& Color);
    const FString& GetBaseColorTexturePath() const;
    bool SetBaseColorTexturePath(const FString& Path);
    bool IsTwoSided() const;
    bool SetTwoSided(bool TwoSided);
    UTexture2D* GetBaseColorTexture() const;
    bool ResolveTexture(FAssetManager& Assets, FString& Error);
    bool CreateResources(ID3D11Device* Device, ID3D11DeviceContext* Context);
    void ReleaseResources();
    ID3D11Buffer* GetConstantBuffer() const;

private:
    ReflectMembers();

private:
    FString mShaderPath{"Shaders/Lit.hlsl"};
    ReflectAccessor(mShaderPath, GetShaderPath, SetShaderPath, "Shader", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 표시 색상입니다.
     */
    FVector4 mBaseColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mBaseColor, GetBaseColor, SetBaseColor, "BaseColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    FString mBaseColorTexturePath{};
    ReflectAccessor(mBaseColorTexturePath, GetBaseColorTexturePath, SetBaseColorTexturePath, "BaseColorTexture", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    bool mTwoSided{};
    ReflectAccessor(mTwoSided, IsTwoSided, SetTwoSided, "TwoSided", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    UTexture2D* mBaseColorTexture{};
    ReflectProperty(mBaseColorTexture, "ResolvedBaseColorTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    Microsoft::WRL::ComPtr<ID3D11Buffer> mConstantBuffer{};
    ReflectProperty(mConstantBuffer, "ConstantBuffer", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    bool mConstantsDirty{true};
    ReflectProperty(mConstantsDirty, "ConstantsDirty", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
