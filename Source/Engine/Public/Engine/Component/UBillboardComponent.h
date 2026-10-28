/**
 * @file UBillboardComponent.h
 * @brief 카메라 정렬과 고정 평면 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/FDrawSprite.h"

/** @brief 카메라를 향하는 텍스처 스프라이트이며 비활성 정렬 시 로컬 YZ 평면을 사용합니다. */
class UBillboardComponent : public UPrimitiveComponent {
public:
    ReflectTypes(UBillboardComponent, UPrimitiveComponent);

public:
    /**
     * @brief 전달된 설정과 기본값으로 UBillboardComponent 객체를 생성합니다.
     */
    UBillboardComponent();
    /**
     * @brief 객체를 소멸하고 소유한 GPU 자원 또는 UV 제어 상태를 해제합니다.
     */
    ~UBillboardComponent() override;

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UBillboardComponent);

    /**
     * @brief Content 기준 텍스처 에셋 경로이며 빈 경로는 기본 아틀라스를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FString& GetTexturePath() const;
    /**
     * @brief Content 기준 텍스처 에셋 경로이며 빈 경로는 기본 아틀라스를 설정합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetTexturePath(const FString& Path);
    /**
     * @brief 저장된 텍스처 경로를 에셋 관리자에서 해석하고 렌더 상태를 갱신합니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ResolveTexture();
    /**
     * @brief 텍스처의 비소유 주소를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    UTexture2D* GetTexture() const;

    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector2& GetSize() const;
    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기를 설정합니다.
     * @param[in] Size 쿼드의 크기 또는 뷰포트 픽셀 크기입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetSize(const FVector2& Size);
    /**
     * @brief 표시할 RGBA 색상을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector4& GetColor() const;
    /**
     * @brief 표시할 RGBA 색상을 설정합니다.
     * @param[in] Color 적용할 색상입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetColor(const FVector4& Color);
    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부인지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsScreenAligned() const;
    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부를 설정합니다.
     * @param[in] ScreenAligned 카메라 축에 정렬할지 지정합니다.
     */
    void SetScreenAligned(bool ScreenAligned);
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
    void UnregisterComponent() override;

private:
    ReflectMembers();
    void OnRegister() override;
    /**
     * @brief Content 상대 경로의 텍스처를 에셋 관리자에서 조회합니다.
     * @param[in] Path Content 상대 에셋 경로 또는 작업할 파일 경로입니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    UTexture2D* LoadTexture(const FString& Path) const;

private:
    /**
     * @brief Content 기준 텍스처 에셋 경로이며 빈 경로는 기본 아틀라스입니다.
     */
    FString mTexturePath{};
    ReflectAccessor(mTexturePath, GetTexturePath, SetTexturePath, "Texture", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 로컬 쿼드의 가로·세로 월드 크기입니다.
     */
    FVector2 mSize{2.0f, 2.0f};
    ReflectAccessor(mSize, GetSize, SetSize, "Size", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 표시할 RGBA 색상입니다.
     */
    FVector4 mColor{1.0f, 1.0f, 1.0f, 1.0f};
    ReflectAccessor(mColor, GetColor, SetColor, "Color", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 카메라의 오른쪽·위쪽 축에 쿼드를 맞출지 여부입니다.
     */
    bool mScreenAligned{true};
    ReflectAccessor(mScreenAligned, IsScreenAligned, SetScreenAligned, "ScreenAligned", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 표시 활성 상태입니다.
     */
    bool mVisible{true};
    ReflectAccessor(mVisible, IsVisible, SetVisible, "Visible", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 텍스처의 비소유 주소입니다.
     */
    UTexture2D* mTexture{};
    ReflectProperty(mTexture, "ResolvedTexture", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
