/**
 * @file UBillboardTextComponent.h
 * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Core/Math/FVector4.h"
#include "UBillboardTextComponent.generated.h"

namespace Engine {

    /**
     * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
     */
    UCLASS()
    class UBillboardTextComponent final : public Engine::UPrimitiveComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 전달된 설정과 기본값으로 UBillboardTextComponent 객체를 생성합니다.
         */
        UBillboardTextComponent();

        /**
         * @brief 기본 소멸자이며, 렌더 장면 등록 해제는 UPrimitiveComponent 소멸자가 처리합니다.
         */
        ~UBillboardTextComponent() override;

    public:
        /**
         * @brief UTF-8 문자열을 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FString& GetText() const;

        /**
         * @brief UTF-8 문자열을 설정합니다.
         * @param[in] Text UTF-8 문자열입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetText(const Core::FString& Text);

        /**
         * @brief 월드 텍스트의 글자 크기를 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        float GetTextSize() const;

        /**
         * @brief 월드 텍스트의 글자 크기를 설정합니다.
         * @param[in] Size 글자 크기입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetTextSize(float Size);

        /**
         * @brief 글자의 RGBA 색상을 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const Core::FVector4& GetTextColor() const;

        /**
         * @brief 글자의 RGBA 색상을 설정합니다.
         * @param[in] Color 적용할 색상입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
         */
        bool SetTextColor(const Core::FVector4& Color);

        bool GetDepthTest() const;
        bool SetDepthTest(bool DepthTest);

        /**
         * @brief 표시할 프리미티브 메시 종류를 반환합니다.
         * @return 표시할 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType GetPrimitiveType() const override;

        /**
         * @brief 메시의 CPU 정점 속성·인덱스 데이터 참조를 반환합니다.
         * @return 객체가 보유한 데이터의 참조이며 소유 객체의 수명 동안 유효합니다.
         */
        const Asset::FMeshData& GetMeshData() const override;

        /**
         * @brief 컴포넌트의 현재 상태를 복사한 프리미티브 프록시를 만듭니다.
         * @return 현재 상태를 담은 프록시입니다.
         */
        Engine::FPrimitiveSceneProxy CreateSceneProxy() const override;

        /**
         * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
         * @return 월드 공간의 축 정렬 경계 상자입니다.
         */
        Core::FBox GetWorldBounds() const override;

        /**
         * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool ShouldCreateRenderState() const override;

    private:
        /**
         * @brief UTF-8 문자열입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetText, Setter = SetText)
        Core::FString mText{"Text"};

        /**
         * @brief 월드 텍스트의 글자 크기입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetTextSize, Setter = SetTextSize)
        float mTextSize{0.5f};

        /**
         * @brief 글자의 RGBA 색상입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetTextColor, Setter = SetTextColor)
        Core::FVector4 mTextColor{1.0f, 1.0f, 1.0f, 1.0f};

        UPROPERTY(Edit, Serialize, Getter = GetDepthTest, Setter = SetDepthTest)
        bool mDepthTest{true};
    };

}
