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

    /**
     * @brief 전달된 설정과 기본값으로 UBillboardTextComponent 객체를 생성합니다.
     */
    Engine::UBillboardTextComponent::UBillboardTextComponent() = default;

    /**
     * @brief 기본 소멸자이며, 렌더 장면 등록 해제는 UPrimitiveComponent 소멸자가 처리합니다.
     */
    Engine::UBillboardTextComponent::~UBillboardTextComponent() = default;

    /**
     * @brief UTF-8 문자열을 반환합니다.
     */
    const Core::FString& Engine::UBillboardTextComponent::GetText() const {
        return mText;
    }

    /**
     * @brief UTF-8 문자열을 설정합니다.
     */
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

    /**
     * @brief 월드 텍스트의 글자 크기를 반환합니다.
     */
    float Engine::UBillboardTextComponent::GetTextSize() const {
        return mTextSize;
    }

    /**
     * @brief 월드 텍스트의 글자 크기를 설정합니다.
     */
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

    /**
     * @brief 글자의 RGBA 색상을 반환합니다.
     */
    const Core::FVector4& Engine::UBillboardTextComponent::GetTextColor() const {
        return mTextColor;
    }

    /**
     * @brief 글자의 RGBA 색상을 설정합니다.
     */
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

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     */
    Engine::EPrimitiveType Engine::UBillboardTextComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Plane;
    }

    /**
     * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
     */
    const Asset::FMeshData& Engine::UBillboardTextComponent::GetMeshData() const {
        static const Asset::FMeshData Mesh{};

        return Mesh;
    }

    /**
     * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
     */
    Engine::FPrimitiveSceneProxy Engine::UBillboardTextComponent::CreateSceneProxy() const {
        Engine::FPrimitiveSceneProxy Proxy{GetGuid(), GetPrimitiveType(), GetWorldMatrix(), GetWorldBounds()};
        Proxy.mText = Engine::FDrawText{{}, mText, mTextColor, mTextSize};

        return Proxy;
    }

    /**
     * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
     */
    Core::FBox Engine::UBillboardTextComponent::GetWorldBounds() const {
        const Core::FTransform& Transform{GetTransform()};
        const float Radius{mTextSize * static_cast<float>(mText.size() + 1) * 2.0f * std::max(std::abs(Transform.mScale.y), std::abs(Transform.mScale.z))};
        const Core::FVector Extent{Radius, Radius, Radius};

        return {Transform.mLocation - Extent, Transform.mLocation + Extent};
    }

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     */
    bool Engine::UBillboardTextComponent::ShouldCreateRenderState() const {
        return !mText.empty() && mTextColor.w > 0.0f;
    }

}
