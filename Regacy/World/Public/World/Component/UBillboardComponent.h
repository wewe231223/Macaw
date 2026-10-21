#pragma once
#include "CoreUObject/Reflection/ReflectionMacros.h"
#include "World/Component/UPrimitiveComponent.h"
#include "Core/Asset/FAssetReference.h"
#include <array>
#include "Math/FMath.h"
#include "Asset/UTexture.h"
#include "Asset/Pipeline/UPipeline.h"

class FProperty;

// 카메라를 향하는 Primitive의 공통 기반 클래스.
// Billboard의 실제 방향 계산은 Shader에서 수행한다.
// 이 클래스는 렌더링 여부와 Billboard 원점만 제공한다.
class UBillboardComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UBillboardComponent, UPrimitiveComponent);

public:
    UBillboardComponent();
    ~UBillboardComponent() override = default;

public:
    ReflectClass(UBillboardComponent, EClassFlags::None);

    // Sprite
    void SetTextureHandle(FAssetHandle InTextureHandle);
    void SetPipelineHandle(FAssetHandle InPipelineHandle);
    void SetSize(const FVector2& InSize);
    void SetUV(const FVector2& InUVMin, const FVector2& InUVMax);
    void SetColor(const FVector4& InColor);

    FAssetHandle GetTextureHandle() const;
    FAssetHandle GetPipelineHandle() const;
    const FVector2& GetSize() const;
    const FVector2& GetUVmin() const;
    const FVector2& GetUVMax() const;
    const FVector4& GetColor() const;

    void OnRegister() override;
    bool BuildBillboardDrawData(FBillboardDrawData& OutData) const;
    bool GetWorldCorners(const FMatrix& CameraWorld, std::array<FVector3, 4>& OutCorners) const;

    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;
    void SendRenderTransform() override;
    void SendRenderDynamicData() override;

private:
    //Billboard를 현재 프레임에 렌더할 수 있는지 검사한다.
    bool CanRenderBillBoard() const;

    void Serialize(FArchive& Archive) override;

    // Billboard 렌더링에 사용할 World Transform을 반환한다.
    // 기본 구현 : 자신의 ComponentToWorld 사용
    virtual bool TryGetBillBoardWorld(FMatrix& OutWorld) const;

    static void SerializeTextureHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);
    static void SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property);

private:
    ReflectAccessor(mTextureReference, GetTextureHandle, SetTextureHandle, "TextureHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializeTextureHandleProperty, ReflectEditor("Billboard", "Texture"), ReflectAsset(UTexture));
    FAssetReference mTextureReference{};
    ReflectAccessor(mPipelineReference, GetPipelineHandle, SetPipelineHandle, "PipelineHandle", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Asset, &SerializePipelineHandleProperty, ReflectEditor("Billboard", "Pipeline"), ReflectAsset(UPipeline));
    FAssetReference mPipelineReference{};
    ReflectAccessor(mSize, GetSize, SetSize, "Size", EPropertyFlags::Edit | EPropertyFlags::Serialize, ReflectSerializer(mSize), ReflectEditor("Billboard", "Size"), ReflectRange(0.001f, std::numeric_limits<float>::max()));
    FVector2 mSize{1.0f, 1.0f};
    ReflectReadOnlyAccessor(mUvMin, GetUVmin, "UVMin", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mUvMin), ReflectEditor("Billboard", "UV Min"));
    FVector2 mUvMin{0.0f, 0.0f};
    ReflectReadOnlyAccessor(mUvMax, GetUVMax, "UVMax", EPropertyFlags::Serialize | EPropertyFlags::ReadOnly, ReflectSerializer(mUvMax), ReflectEditor("Billboard", "UV Max"));
    FVector2 mUvMax{1.0f, 1.0f};
    ReflectAccessor(mColor, GetColor, SetColor, "Color", EPropertyFlags::Edit | EPropertyFlags::Serialize | EPropertyFlags::Color, ReflectSerializer(mColor), ReflectEditor("Billboard", "Color"));
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};

    ReflectMembers();
};
