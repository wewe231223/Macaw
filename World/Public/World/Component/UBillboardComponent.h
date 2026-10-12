#pragma once
#include "World/Component/UPrimitiveComponent.h"
#include "Core/Asset/FAssetPath.h"
#include <array>
#include "Math/FMath.h"
#include "World/Component/UBillboardComponent.generated.h"

// 카메라를 향하는 Primitive의 공통 기반 클래스.
// Billboard의 실제 방향 계산은 Shader에서 수행한다.
// 이 클래스는 렌더링 여부와 Billboard 원점만 제공한다.
UCLASS()
class UBillboardComponent : public UPrimitiveComponent {
public:
    GENERATED_TYPES(UBillboardComponent)

public:
    UBillboardComponent();
    ~UBillboardComponent() override = default;

public:
    GENERATED_BODY(UBillboardComponent)

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

    bool BuildBillboardDrawData(FBillboardDrawData& OutData) const;
    bool GetWorldCorners(const FMatrix& CameraWorld, std::array<FVector3, 4>& OutCorners) const;

    bool ShouldCreateRenderState() const override;
    std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const override;
    void SendRenderTransform() override;
    void SendRenderDynamicData() override;

protected:
    //Billboard를 현재 프레임에 렌더할 수 있는지 검사한다.
    bool CanRenderBillBoard() const;

    void Serialize(FArchive& Archive) override;

    // Billboard 렌더링에 사용할 World Transform을 반환한다.
    // 기본 구현 : 자신의 ComponentToWorld 사용
    virtual bool TryGetBillBoardWorld(FMatrix& OutWorld) const;

private:
    FAssetHandle mTextureHandle{};
    FAssetHandle mPipelineHandle{};
    FAssetPath mTextureAssetPath{};
    FAssetPath mPipelineAssetPath{};
    FGuid mTextureAssetGuid{};
    FGuid mPipelineAssetGuid{};

    UPROPERTY(Serialize, Name = "Size", Getter = GetSize, Category = "Billboard", DisplayName = "Size", Edit, Setter = SetSize, Min = 0.001f)
    FVector2 mSize{1.0f, 1.0f};
    UPROPERTY(Serialize, Name = "UVMin", Getter = GetUVmin, Category = "Billboard", DisplayName = "UV Min", ReadOnly)
    FVector2 mUvMin{0.0f, 0.0f};
    UPROPERTY(Serialize, Name = "UVMax", Getter = GetUVMax, Category = "Billboard", DisplayName = "UV Max", ReadOnly)
    FVector2 mUvMax{1.0f, 1.0f};
    UPROPERTY(Serialize, Name = "Color", Getter = GetColor, Category = "Billboard", DisplayName = "Color", Edit, Setter = SetColor, Color)
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
};
