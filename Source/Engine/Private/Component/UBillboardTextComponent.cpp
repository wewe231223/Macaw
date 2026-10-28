/**
 * @file UBillboardTextComponent.cpp
 * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Engine/FMeshData.h"
#include "Core/Math/FTransform.h"

#include <algorithm>
#include <cmath>

UBillboardTextComponent::UBillboardTextComponent() = default;

UBillboardTextComponent::~UBillboardTextComponent() = default;

const FString& UBillboardTextComponent::GetText() const {
    return mText;
}

bool UBillboardTextComponent::SetText(const FString& Text) {
    if (Text.size() > 65536 || Text.find('\0') != FString::npos) {
        return false;
    }
    if (mText != Text) {
        mText = Text;
        MarkRenderStateDirty();
    }
    return true;
}

float UBillboardTextComponent::GetTextSize() const {
    return mTextSize;
}

bool UBillboardTextComponent::SetTextSize(float Size) {
    if (!std::isfinite(Size) || Size <= 0.0f || Size > 10000.0f) {
        return false;
    }
    if (mTextSize != Size) {
        mTextSize = Size;
        MarkRenderStateDirty();
    }
    return true;
}

const FVector4& UBillboardTextComponent::GetTextColor() const {
    return mTextColor;
}

bool UBillboardTextComponent::SetTextColor(const FVector4& Color) {
    const auto Values{Color.ToArray()};
    if (!std::all_of(Values.begin(), Values.end(), [](float Value) {
        return std::isfinite(Value) && Value >= 0.0f && Value <= 1.0f;
    })) {
        return false;
    }
    mTextColor = Color;
    MarkRenderStateDirty();
    return true;
}

bool UBillboardTextComponent::IsVisible() const {
    return mVisible;
}

void UBillboardTextComponent::SetVisible(bool Visible) {
    if (mVisible != Visible) {
        mVisible = Visible;
        MarkRenderStateDirty();
    }
}

EPrimitiveType UBillboardTextComponent::GetPrimitiveType() const {
    return EPrimitiveType::Plane;
}

const FMeshData& UBillboardTextComponent::GetMeshData() const {
    static const FMeshData Mesh{};
    return Mesh;
}

FPrimitiveSceneProxy UBillboardTextComponent::CreateSceneProxy() const {
    FPrimitiveSceneProxy Proxy{GetGuid(), GetPrimitiveType(), GetWorldMatrix(), GetWorldBounds()};
    Proxy.mText = FDrawText{{}, mText, mTextColor, mTextSize};
    return Proxy;
}

FBox UBillboardTextComponent::GetWorldBounds() const {
    const FTransform& Transform{GetTransform()};
    const float Radius{mTextSize * static_cast<float>(mText.size() + 1) * 2.0f * std::max(std::abs(Transform.mScale.y), std::abs(Transform.mScale.z))};
    const FVector Extent{Radius, Radius, Radius};
    return {Transform.mLocation - Extent, Transform.mLocation + Extent};
}

bool UBillboardTextComponent::ShouldCreateRenderState() const {
    return mVisible && !mText.empty() && mTextColor.w > 0.0f;
}
