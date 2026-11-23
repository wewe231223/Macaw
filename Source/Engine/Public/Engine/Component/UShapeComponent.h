/**
 * @file UShapeComponent.h
 * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
 * @ingroup Engine
 */

#pragma once

#include "Engine/Component/UPrimitiveComponent.h"
#include "Engine/Collision/FCollisionShape.h"
#include "Engine/Collision/FHitResult.h"
#include "CoreUObject/TDelegate.h"
#include "Core/Math/FVector4.h"
#include "Core/Math/FBox.h"
#include "Core/Literals/QuantityTypes.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "PhysicsRuntime/EBodyType.h"
#include "PhysicsRuntime/FBodyInstance.h"
#include "PhysicsRuntime/UBodySetup.h"
#include "PhysicsRuntime/UPhysicalMaterial.h"
#include <memory>
#include "UShapeComponent.generated.h"

namespace Engine {

    /**
     * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
     */
    UCLASS(Abstract)
    class UShapeComponent : public Engine::UPrimitiveComponent {
        GENERATED_BODY()

    public:
        /**
         * @brief 겹친 컴포넌트를 전달하는 이벤트 시그니처입니다.
         */
        using FOverlapDelegate = CoreUObject::TDelegate<Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*>;

        /**
         * @brief 충돌한 액터·컴포넌트·충돌 정보를 전달하는 이벤트 시그니처입니다.
         */
        using FHitDelegate = CoreUObject::TDelegate<Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*, const Engine::FHitResult&>;

    public:
        /**
         * @brief 전달된 설정과 기본값으로 UShapeComponent 객체를 생성합니다.
         */
        UShapeComponent();

    public:
        void BeginPlay() override;
        void EndPlay() override;

        bool IsPhysicsEnabled() const;
        bool SetPhysicsEnabled(bool Enabled);

        PhysicsRuntime::EBodyType GetBodyType() const;
        bool SetBodyType(PhysicsRuntime::EBodyType BodyType);

        void Serialize(Core::FArchive& Archive) override;

        const Core::FString& GetPhysicalMaterialPath() const;
        bool SetPhysicalMaterialPath(const Core::FString& Path);
        const PhysicsRuntime::UPhysicalMaterial& GetPhysicalMaterial() const;

        const PhysicsRuntime::UBodySetup& GetBodySetup();
        bool HasPhysicsBody() const;
        void ReleasePhysicsBody();

        void PushKinematicTarget(float Alpha);
        void PullPhysicsTransform(bool MoveOwner);

        /**
         * @brief 컴포넌트 겹침 시작 이벤트를 반환합니다.
         * @return 이 컴포넌트가 소유한 이벤트 델리게이트의 참조입니다.
         */
        FOverlapDelegate& GetOnComponentBeginOverlap();

        /**
         * @brief 컴포넌트 겹침 종료 이벤트를 반환합니다.
         * @return 이 컴포넌트가 소유한 이벤트 델리게이트의 참조입니다.
         */
        FOverlapDelegate& GetOnComponentEndOverlap();

        /**
         * @brief 컴포넌트 충돌 이벤트를 반환합니다.
         * @return 이 컴포넌트가 소유한 이벤트 델리게이트의 참조입니다.
         */
        FHitDelegate& GetOnComponentHit();

        bool BlocksMovement() const;
        void SetBlocksMovement(bool Blocks);

        /**
         * @brief 겹침 시작 델리게이트를 호출합니다.
         * @param[in] Other 비교할 상대 값입니다.
         */
        void OnBeginOverlap(Engine::UShapeComponent* Other);

        /**
         * @brief 겹침 종료 델리게이트를 호출합니다.
         * @param[in] Other 비교할 상대 값입니다.
         */
        void OnEndOverlap(Engine::UShapeComponent* Other);

        /**
         * @brief 적중 액터·컴포넌트와 충돌 정보를 델리게이트로 전달합니다.
         * @param[in] OtherActor 충돌한 상대 액터입니다.
         * @param[in] OtherComp 충돌한 상대 컴포넌트입니다.
         * @param[in] Hit 원본 충돌 결과입니다.
         */
        void OnHit(Engine::AActor* OtherActor, Engine::UPrimitiveComponent* OtherComp, const Engine::FHitResult& Hit);

        /**
         * @brief 검사에 사용할 충돌 형상 값을 반환합니다.
         * @return 계산하거나 조회한 값입니다.
         */
        virtual Engine::FCollisionShape GetCollisionShape() const = 0;

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

        /**
         * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
         * @return 월드 공간의 축 정렬 경계 상자입니다.
         */
        FBox GetWorldBounds() const override;

        /**
         * @brief 표시할 프리미티브 메시 종류를 반환합니다.
         * @return 표시할 프리미티브 메시 종류입니다.
         */
        Engine::EPrimitiveType GetPrimitiveType() const override;

        /**
         * @brief 충돌 형상을 선택 강조 색상 또는 기본 색상의 선으로 수집합니다.
         * @param[in] DrawInterface 보조 선분을 추가할 프레임 수집기입니다.
         * @param[in] Highlight 선택 강조 색상을 사용할지 지정합니다.
         */
        void DrawDebug(Engine::FPrimitiveDrawInterface& DrawInterface, bool Highlight) const;

        /**
         * @brief 두 컴포넌트의 월드 충돌 형상이 겹치는지 검사합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool IsOverlapping(const Engine::UShapeComponent& Other) const;

        /**
         * @brief 두 형상의 월드 접촉 법선·깊이·위치를 계산합니다.
         * @param[in] Other 비교할 상대 값입니다.
         * @param[out] OutContact 접촉 정보를 기록할 선택적 출력 주소입니다.
         * @return 두 형상이 겹쳐 접촉 정보를 계산했으면 true입니다.
         */
        bool ComputeContact(const Engine::UShapeComponent& Other, Engine::FCollisionContact& OutContact) const;

        /**
         * @brief 충돌 도형을 표시할 RGBA 색상을 반환합니다.
         * @return 이 객체가 보관하는 값의 참조입니다.
         */
        const FVector4& GetShapeColor() const;

        /**
         * @brief 충돌 도형을 표시할 RGBA 색상을 설정합니다.
         * @param[in] Color 적용할 색상입니다.
         * @return 값이 유효해 적용했으면 true, 거부했으면 false입니다.
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
         * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
         * @return 조건을 만족하면 true입니다.
         */
        bool ShouldCreateRenderState() const override;

        const PhysicsRuntime::UPhysicalMaterial& GetDefaultPhysicalMaterial() const;
        physx::PxTransform MakePhysicsPose() const;
        std::unique_ptr<PhysicsRuntime::FBodyInstance> CreatePhysicsBody(PhysicsRuntime::EBodyType BodyType);

        void AddArc(Engine::FPrimitiveDrawInterface& DrawInterface, const FVector& C, const FVector& U, const FVector& V, float R, Core::FAngle StartAngle, Core::FAngle EndAngle, const FVector& Color) const;

    private:
        /**
         * @brief 충돌 도형을 표시할 RGBA 색상입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = GetShapeColor, Setter = SetShapeColor)
        FVector4 mShapeColor{223.0f / 255.0f, 149.0f / 255.0f, 157.0f / 255.0f, 1.0f};

        /**
         * @brief 선택한 경우에만 충돌 도형을 표시할지 여부입니다.
         */
        UPROPERTY(Edit, Serialize, Getter = ShouldDrawOnlyIfSelected, Setter = SetDrawOnlyIfSelected)
        bool mDrawOnlyIfSelected{};

        /**
         * @brief 컴포넌트 겹침 시작 이벤트입니다.
         */
        FOverlapDelegate mOnComponentBeginOverlap{};

        /**
         * @brief 컴포넌트 겹침 종료 이벤트입니다.
         */
        FOverlapDelegate mOnComponentEndOverlap{};

        /**
         * @brief 컴포넌트 충돌 이벤트입니다.
         */
        FHitDelegate mOnComponentHit{};

        UPROPERTY(Edit, Serialize, Getter = BlocksMovement, Setter = SetBlocksMovement)
        bool mBlocksMovement{};

        UPROPERTY(Edit, Serialize, Getter = IsPhysicsEnabled, Setter = SetPhysicsEnabled)
        bool mPhysicsEnabled{};

        UPROPERTY(Edit, Serialize, Getter = GetBodyType, Setter = SetBodyType)
        PhysicsRuntime::EBodyType mBodyType{PhysicsRuntime::EBodyType::Static};

        UPROPERTY(Edit, Serialize, Name = "PhysicalMaterial", Getter = GetPhysicalMaterialPath, Setter = SetPhysicalMaterialPath)
        Core::FString mPhysicalMaterialPath{};

        std::unique_ptr<PhysicsRuntime::UPhysicalMaterial> mLegacyPhysicalMaterial{};
        std::unique_ptr<PhysicsRuntime::UBodySetup> mBodySetup{};
        std::unique_ptr<PhysicsRuntime::FBodyInstance> mBodyInstance{};
    };

}
