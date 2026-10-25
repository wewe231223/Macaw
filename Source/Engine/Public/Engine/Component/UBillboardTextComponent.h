#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Core/Math/FVector4.h"

class UBillboardTextComponent final : public UPrimitiveComponent {
public:
    ReflectTypes(UBillboardTextComponent, UPrimitiveComponent);

public:
    UBillboardTextComponent();
    ~UBillboardTextComponent() override;

public:
    ReflectClass(UBillboardTextComponent);

    const FString& GetText() const;
    bool SetText(const FString& Text);
    float GetTextSize() const;
    bool SetTextSize(float Size);
    const FVector4& GetTextColor() const;
    bool SetTextColor(const FVector4& Color);
    bool IsVisible() const;
    void SetVisible(bool Visible);
    EPrimitiveType GetPrimitiveType() const override;
    const FMeshData& GetMeshData() const override;
    FPrimitiveSceneProxy CreateSceneProxy() const override;
    FBox GetWorldBounds() const override;
    bool ShouldCreateRenderState() const override;

private:
    ReflectMembers();

private:
    FString mText{"Text"};
    ReflectAccessor(mText, GetText, SetText, "Text", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    float mTextSize{0.5f};
    ReflectAccessor(mTextSize, GetTextSize, SetTextSize, "TextSize", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    FVector4 mTextColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mTextColor, GetTextColor, SetTextColor, "TextColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    bool mVisible{true};
    ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
