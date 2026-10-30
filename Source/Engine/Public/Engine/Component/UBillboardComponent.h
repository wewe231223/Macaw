/**
 * @file UBillboardComponent.h
 * @brief 카메라 정렬과 고정 평면 텍스처 스프라이트를 제공합니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/FDrawSprite.h"

class UTexture;

/**
 * @brief 카메라를 향하는 텍스처 스프라이트이며 비활성 정렬 시 로컬 YZ 평면을 사용합니다.
 */
class UBillboardComponent : public UPrimitiveComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
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
     * @brief 빌보드에 할당된 Sprite 텍스처를 반환합니다.
     * @return 에셋 관리자가 소유한 텍스처의 비소유 주소이며 미할당 또는 해석 실패 시 nullptr입니다.
     */
    UTexture* GetSprite() const;

    /**
     * @brief 현재 월드의 에셋 관리자가 보유한 2D 텍스처를 Sprite에 할당하고 저장할 경로를 갱신합니다.
     * @param[in] Sprite 할당할 텍스처의 비소유 주소이며 nullptr이면 기본 아틀라스로 복원합니다.
     * @return 할당 또는 초기화에 성공하면 true이며 유효하지 않거나 관리하지 않는 텍스처이면 false입니다.
     */
    bool SetSprite(UTexture* Sprite);

    /**
     * @brief Sprite 텍스처의 Content 기준 에셋 경로를 반환합니다.
     * @return 객체가 보관하는 경로의 참조이며 빈 경로는 기본 아틀라스 사용을 뜻합니다.
     */
    const FString& GetTexturePath() const;

    /**
     * @brief 이미지 또는 .masset 텍스처의 Content 상대 경로를 검증하고 Sprite를 갱신합니다.
     * @param[in] Path 할당할 텍스처 경로이며 빈 문자열이면 기본 아틀라스로 복원합니다.
     * @return 경로를 수락하면 true이며 경로 검증 또는 연결된 에셋 관리자에서의 로드에 실패하면 false입니다.
     * @note 에셋 관리자가 연결되지 않았으면 경로를 보관하고 등록 또는 에셋 연결 시 텍스처를 해석합니다.
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

    /**
     * @brief 빌보드 선택과 경계 계산에 사용하는 기본 도형 종류를 반환합니다.
     * @return 스프라이트 쿼드에 대응하는 평면 도형 종류입니다.
     */
    EPrimitiveType GetPrimitiveType() const override;

    /**
     * @brief 별도 스프라이트 렌더러를 사용하는 빌보드의 빈 메시 데이터를 반환합니다.
     * @return 프로세스 수명 동안 유지되는 빈 메시 데이터의 참조입니다.
     */
    const FMeshData& GetMeshData() const override;

    /**
     * @brief Sprite 텍스처, 크기, 색상과 카메라 정렬 설정을 담은 렌더 프록시를 생성합니다.
     * @return 현재 빌보드의 변환과 표시 정보를 복사한 프록시입니다.
     */
    FPrimitiveSceneProxy CreateSceneProxy() const override;

    /**
     * @brief 카메라 정렬 방향에 관계없이 빌보드를 포함하는 월드 경계를 계산합니다.
     * @return 쿼드의 중심과 대각선 반경을 기준으로 계산한 월드 AABB입니다.
     */
    FBox GetWorldBounds() const override;

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool ShouldCreateRenderState() const override;

    /**
     * @brief 렌더 상태와 월드 등록을 해제하고 Sprite의 비소유 참조를 비웁니다.
     */
    void UnregisterComponent() override;

    /**
     * @brief 빌보드 프로퍼티를 직렬화하고 이전 Texture 필드를 Sprite 경로로 복원합니다.
     * @param[in,out] Archive 저장 또는 로드할 아카이브이며 이전 텍스처 경로가 잘못되면 오류를 기록합니다.
     */
    void Serialize(FArchive& Archive) override;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();

    /**
     * @brief 월드에 등록할 때 저장된 Sprite 경로를 해석하고 렌더 상태 갱신을 예약합니다.
     */
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
    ReflectAccessor(mTexturePath, GetTexturePath, SetTexturePath, "Sprite", EPropertyFlags::Edit | EPropertyFlags::Serialize);

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
    UTexture* mSprite{};
    ReflectProperty(mSprite, "ResolvedSprite", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
