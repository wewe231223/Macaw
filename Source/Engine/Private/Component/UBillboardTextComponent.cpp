/**
 * @file UBillboardTextComponent.cpp
 * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
 * @ingroup Engine
 */

#include "Engine/Component/UBillboardTextComponent.h"
#include "Engine/FPrimitiveSceneProxy.h"
#include "Asset/FMeshData.h"
#include "Core/Math/FTransform.h"

#include <algorithm>
#include <cmath>

namespace Engine {

    Engine::UBillboardTextComponent::UBillboardTextComponent() = default;

    Engine::UBillboardTextComponent::~UBillboardTextComponent() = default;

    const Core::FString& Engine::UBillboardTextComponent::GetText() const {
        return mText;
    }

    bool Engine::UBillboardTextComponent::SetText(const Core::FString& Text) {
        if (Text.size() > 65536 || Text.find('\0') != Core::FString::npos) {
            return false;
        }

        if (mText != Text) {
            mText = Text;
            MarkRenderStateDirty();
        }

        return true;
    }

    float Engine::UBillboardTextComponent::GetTextSize() const {
        return mTextSize;
    }

    bool Engine::UBillboardTextComponent::SetTextSize(float Size) {
        if (!std::isfinite(Size) || Size <= 0.0f || Size > 10000.0f) {
            return false;
        }

        if (mTextSize != Size) {
            mTextSize = Size;
            MarkRenderStateDirty();
        }

        return true;
    }

    const Core::FVector4& Engine::UBillboardTextComponent::GetTextColor() const {
        return mTextColor;
    }

    bool Engine::UBillboardTextComponent::SetTextColor(const Core::FVector4& Color) {
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

    bool Engine::UBillboardTextComponent::IsVisible() const {
        return mVisible;
    }

    void Engine::UBillboardTextComponent::SetVisible(bool Visible) {
        if (mVisible != Visible) {
            mVisible = Visible;
            MarkRenderStateDirty();
        }
    }

    Engine::EPrimitiveType Engine::UBillboardTextComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Plane;
    }

    const Asset::FMeshData& Engine::UBillboardTextComponent::GetMeshData() const {
        static const Asset::FMeshData Mesh{};
        return Mesh;
    }

    Engine::FPrimitiveSceneProxy Engine::UBillboardTextComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{GetGuid(), GetPrimitiveType(), GetWorldMatrix(), GetWorldBounds()};
        Proxy.mText = Engine::FDrawText{{}, mText, mTextColor, mTextSize};
        return Proxy;
    }

    Core::FBox Engine::UBillboardTextComponent::GetWorldBounds() const {
        const Core::FTransform& Transform{GetTransform()};
        const float Radius{mTextSize * static_cast<float>(mText.size() + 1) * 2.0f * std::max(std::abs(Transform.mScale.y), std::abs(Transform.mScale.z))};
        const Core::FVector Extent{Radius, Radius, Radius};
        return {Transform.mLocation - Extent, Transform.mLocation + Extent};
    }

    bool Engine::UBillboardTextComponent::ShouldCreateRenderState() const {
        return mVisible && !mText.empty() && mTextColor.w > 0.0f;
    }

}
