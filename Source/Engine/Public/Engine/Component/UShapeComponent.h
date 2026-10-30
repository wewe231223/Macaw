/**
 * @file UShapeComponent.h
 * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Collision/FCollisionShape.h"
#include "Engine/Collision/FHitResult.h"
#include "Core/TDelegate.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FBox.h"

class FPrimitiveDrawInterface;

/**
 * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
 */
class UShapeComponent : public UPrimitiveComponent {
public:
    /**
     * @brief 리플렉션 소유 타입과 선택적 부모 타입의 별칭입니다.
     */
    ReflectTypes(UShapeComponent, UPrimitiveComponent);
    /**
     * @brief 겹친 컴포넌트를 전달하는 이벤트 시그니처입니다.
     */
    using FOverlapDelegate = TDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*)>;
    /**
     * @brief 충돌한 액터·컴포넌트·충돌 정보를 전달하는 이벤트 시그니처입니다.
     */
    using FHitDelegate = TDelegate<void(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, const FHitResult&)>;

public:
    /**
     * @brief 전달된 설정과 기본값으로 UShapeComponent 객체를 생성합니다.
     */
    UShapeComponent();

public:
    /**
     * @brief 이 타입의 정적 리플렉션 메타데이터를 반환합니다.
     * @return 조회한 객체 또는 GPU 자원의 비소유 주소이며 사용할 수 없으면 nullptr입니다.
     */
    ReflectClass(UShapeComponent, EClassFlags::Abstract);
    /**
     * @brief 컴포넌트 겹침 시작 이벤트를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    FOverlapDelegate& GetOnComponentBeginOverlap();
    /**
     * @brief 컴포넌트 겹침 종료 이벤트를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    FOverlapDelegate& GetOnComponentEndOverlap();
    /**
     * @brief 컴포넌트 충돌 이벤트를 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    FHitDelegate& GetOnComponentHit();
    /**
     * @brief 겹침 시작 델리게이트를 호출합니다.
     * @param[in] Other 비교하거나 검사할 상대 값입니다.
     */
    void OnBeginOverlap(UShapeComponent* Other);
    /**
     * @brief 겹침 종료 델리게이트를 호출합니다.
     * @param[in] Other 비교하거나 검사할 상대 값입니다.
     */
    void OnEndOverlap(UShapeComponent* Other);
    /**
     * @brief 적중 액터·컴포넌트와 충돌 정보를 델리게이트로 전달합니다.
     * @param[in] OtherActor 충돌한 상대 액터입니다.
     * @param[in] OtherComp 충돌한 상대 컴포넌트입니다.
     * @param[in] Hit 원본 충돌 결과입니다.
     */
    void OnHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FHitResult& Hit);
    /**
     * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    virtual FCollisionShape GetCollisionShape() const = 0;
    /**
     * @brief 충돌 형상의 로컬 축 정렬 경계를 계산합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    virtual FBox CalcLocalBounds() const = 0;
    /**
     * @brief 컴포넌트 변환을 적용한 월드 충돌 경계를 계산합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    FBox CalcBounds() const;
    FBox GetWorldBounds() const override;
    EPrimitiveType GetPrimitiveType() const override;
    /**
     * @brief 충돌 형상을 선택 강조 색상 또는 기본 색상의 선으로 수집합니다.
     * @param[in] DrawInterface 보조 선분을 추가할 프레임 수집기입니다.
     * @param[in] Highlight 선택 강조 색상을 사용할지 지정합니다.
     */
    void DrawDebug(FPrimitiveDrawInterface& DrawInterface, bool Highlight) const;
    /**
     * @brief 두 컴포넌트의 월드 충돌 형상이 겹치는지 검사합니다.
     * @param[in] Other 비교하거나 검사할 상대 값입니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool IsOverlapping(const UShapeComponent& Other) const;
    /**
     * @brief 두 형상의 월드 접촉 법선·깊이·위치를 계산합니다.
     * @param[in] Other 비교하거나 검사할 상대 값입니다.
     * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool ComputeContact(const UShapeComponent& Other, FCollisionContact& OutContact) const;
    /**
     * @brief 충돌 도형을 표시할 RGBA 색상을 반환합니다.
     * @return 이 객체 또는 공유 이름 풀이 유지하는 값의 참조입니다.
     */
    const FVector4& GetShapeColor() const;
    /**
     * @brief 충돌 도형을 표시할 RGBA 색상을 설정합니다.
     * @param[in] Color 적용할 색상입니다.
     * @return 검증 또는 요청한 처리에 성공하면 true입니다.
     */
    bool SetShapeColor(const FVector4& Color);
    /**
     * @brief 선택한 경우에만 보조 충돌 도형을 그릴지 검사합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool ShouldDrawOnlyIfSelected() const;
    /**
     * @brief 선택한 경우에만 충돌 도형을 표시할지 여부를 설정합니다.
     * @param[in] DrawOnlyIfSelected 선택할 때만 보조 도형을 표시할지 지정합니다.
     */
    void SetDrawOnlyIfSelected(bool DrawOnlyIfSelected);
    /**
     * @brief 충돌 형상에 적용할 최대 절대 축 스케일을 반환합니다.
     * @return 계산하거나 조회한 값입니다.
     */
    float GetShapeScale() const;

private:
    /**
     * @brief 생성된 멤버 등록 함수와 리플렉션 빌더의 접근 권한입니다.
     */
    ReflectMembers();
    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     * @return 조건을 만족하면 true입니다.
     */
    bool ShouldCreateRenderState() const override;

private:
    /**
     * @brief 충돌 도형을 표시할 RGBA 색상입니다.
     */
    FVector4 mShapeColor{223.0f / 255.0f, 149.0f / 255.0f, 157.0f / 255.0f, 1.0f};
    ReflectAccessor(mShapeColor, GetShapeColor, SetShapeColor, "ShapeColor", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 선택한 경우에만 충돌 도형을 표시할지 여부입니다.
     */
    bool mDrawOnlyIfSelected{};
    ReflectAccessor(mDrawOnlyIfSelected, ShouldDrawOnlyIfSelected, SetDrawOnlyIfSelected, "DrawOnlyIfSelected", EPropertyFlags::Edit | EPropertyFlags::Serialize);
    /**
     * @brief 컴포넌트 겹침 시작 이벤트입니다.
     */
    FOverlapDelegate mOnComponentBeginOverlap{};
    ReflectProperty(mOnComponentBeginOverlap, "OnComponentBeginOverlap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 컴포넌트 겹침 종료 이벤트입니다.
     */
    FOverlapDelegate mOnComponentEndOverlap{};
    ReflectProperty(mOnComponentEndOverlap, "OnComponentEndOverlap", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
    /**
     * @brief 컴포넌트 충돌 이벤트입니다.
     */
    FHitDelegate mOnComponentHit{};
    ReflectProperty(mOnComponentHit, "OnComponentHit", EPropertyFlags::Transient | EPropertyFlags::ReadOnly);
};
