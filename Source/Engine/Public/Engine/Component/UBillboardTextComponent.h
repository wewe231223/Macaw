/**
 * @file UBillboardTextComponent.h
 * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Core/Math/FVector4.h"

/**
 * @brief 카메라에 정렬한 월드 텍스트를 렌더 장면에 등록합니다.
 */
class UBillboardTextComponent final : public UPrimitiveComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UBillboardTextComponent, UPrimitiveComponent);

public:
    /**
     * @brief 전달된 설정과 기본값으로 UBillboardTextComponent 객체를 생성합니다.
     */
    UBillboardTextComponent();

    /**
     * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
     */
    ~UBillboardTextComponent() override;

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UBillboardTextComponent);

    /**
     * @brief UTF-8 문자열을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FString& GetText() const;

    /**
     * @brief UTF-8 문자열을 설정합니다.
     * @param[in] Text 표시하거나 측정할 UTF-8 문자열 또는 텍스트 요청입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetText(const FString& Text);

    /**
     * @brief 월드 텍스트의 글자 크기를 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetTextSize() const;

    /**
     * @brief 월드 텍스트의 글자 크기를 설정합니다.
     * @param[in] Size 쿼드의 크기 또는 뷰포트 픽셀 크기입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetTextSize(float Size);

    /**
     * @brief 글자의 RGBA 색상을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector4& GetTextColor() const;

    /**
     * @brief 글자의 RGBA 색상을 설정합니다.
     * @param[in] Color 적용할 색상입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetTextColor(const FVector4& Color);

    /**
     * @brief 표시 활성 상태인지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsVisible() const;

    /**
     * @brief 표시 활성 상태를 설정합니다.
     * @param[in] Visible 표시를 활성화할지 지정합니다.
     */
    void SetVisible(bool Visible);
    EPrimitiveType GetPrimitiveType() const override;
    const FMeshData& GetMeshData() const override;
    FPrimitiveSceneProxy CreateSceneProxy() const override;
    FBox GetWorldBounds() const override;

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool ShouldCreateRenderState() const override;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

private:
    /**
     * @brief UTF-8 문자열입니다.
     */
    FString mText{"Text"};
    ReflectAccessor(mText, GetText, SetText, "Text", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 월드 텍스트의 글자 크기입니다.
     */
    float mTextSize{0.5f};
    ReflectAccessor(mTextSize, GetTextSize, SetTextSize, "TextSize", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 글자의 RGBA 색상입니다.
     */
    FVector4 mTextColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mTextColor, GetTextColor, SetTextColor, "TextColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);

    /**
     * @brief 표시 활성 상태입니다.
     */
    bool mVisible{true};
    ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", EPropertyFlags::Edit | EPropertyFlags::Serialize);
};
