/**
 * @file UShapeComponent.cpp
 * @brief 충돌 형상·접촉 이벤트와 에디터 보조 선을 제공하는 기본 컴포넌트입니다.
 * @ingroup Engine
 */

#include "Core/Types.h"
#include "Engine/Component/UShapeComponent.h"
#include "Engine/AActor.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/UWorld.h"
#include "Core/Archive.h"
#include "Core/FilePath.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Core/Math/FBox.h"
#include "Core/Literals.h"
#include "Core/Log/Log.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

using namespace Core::Literals;

namespace Engine {

    DEFINE_LOG_CATEGORY_STATIC(LogShapePhysics, Log, All);

    /**
     * @brief 충돌 도형의 접촉 검사 보조 함수입니다.
     */
    namespace {
        /**
         * @brief 두 AABB의 모든 축 구간이 겹치는지 검사합니다.
         * @param[in] A 첫 번째 검사 대상입니다.
         * @param[in] B 두 번째 검사 대상입니다.
         * @return 두 형상이 겹치면 true입니다.
         */
        bool BoundsOverlap(const FBox& A, const FBox& B) {
            return A.mMinimum.x <= B.mMaximum.x && A.mMaximum.x >= B.mMinimum.x && A.mMinimum.y <= B.mMaximum.y && A.mMaximum.y >= B.mMinimum.y && A.mMinimum.z <= B.mMaximum.z && A.mMaximum.z >= B.mMinimum.z;
        }
    }

    /**
     * @brief 두 기저 방향의 평면에서 원호를 선분으로 수집합니다.
     * @param[in] DrawInterface 보조 선분을 추가할 프레임 수집기입니다.
     * @param[in] C 원호의 중심입니다.
     * @param[in] U 원호 평면의 첫 기저입니다.
     * @param[in] V 원호 평면의 둘째 기저입니다.
     * @param[in] R 원호의 반지름입니다.
     * @param[in] StartAngle 시작 각도입니다.
     * @param[in] EndAngle 끝 각도입니다.
     * @param[in] Color 적용할 색상입니다.
     */
    void Engine::UShapeComponent::AddArc(Engine::FPrimitiveDrawInterface& DrawInterface, const FVector& C, const FVector& U, const FVector& V, float R, Core::FAngle StartAngle, Core::FAngle EndAngle, const FVector& Color) const {
        /**
         * @brief 충돌 도형 원호를 분할할 선분 개수입니다.
         */
        constexpr Int32 CircleSegment{32};
        const Int32 Segments{std::max(1, static_cast<Int32>(CircleSegment * ((EndAngle - StartAngle) / 1_turn)))};
        const Core::FAngle Step{(EndAngle - StartAngle) / Segments};

        FVector Prev{C + U * (Core::Math::Cos(StartAngle) * R) + V * (Core::Math::Sin(StartAngle) * R)};

        for (Index_t Index{1}; Index <= Segments; ++Index) {
            const Core::FAngle Angle{StartAngle + Step * Index};
            const FVector Point{C + U * (Core::Math::Cos(Angle) * R) + V * (Core::Math::Sin(Angle) * R)};

            DrawInterface.DrawLine(Prev, Point, Color);

            Prev = Point;
        }
    }

    /**
     * @brief 전달된 설정과 기본값으로 UShapeComponent 객체를 생성합니다.
     */
    Engine::UShapeComponent::UShapeComponent() {
        mOnComponentBeginOverlap.Add([](Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*) {
            std::clog << "BeginOverlap\n";
        });

        mOnComponentEndOverlap.Add([](Engine::UPrimitiveComponent*, Engine::AActor*, Engine::UPrimitiveComponent*) {
            std::clog << "EndOverlap\n";
        });
    }

    void Engine::UShapeComponent::BeginPlay() {
        Super::BeginPlay();

        if (mPhysicsEnabled && mBodyInstance == nullptr && !SetPhysicsEnabled(true)) {
            MACAW_LOG(LogShapePhysics, Warning, "Could not create the physics body for component {}.", GetName());
        }
    }

    void Engine::UShapeComponent::EndPlay() {
        ReleasePhysicsBody();
        Super::EndPlay();
    }

    bool Engine::UShapeComponent::IsPhysicsEnabled() const {
        return mPhysicsEnabled;
    }

    bool Engine::UShapeComponent::SetPhysicsEnabled(bool Enabled) {
        if (!Enabled) {
            ReleasePhysicsBody();
        } else if (HasBegunPlay() && mBodyInstance == nullptr) {
            mBodyInstance = CreatePhysicsBody(mBodyType);

            if (mBodyInstance == nullptr) {
                return false;
            }
        }

        mPhysicsEnabled = Enabled;

        return true;
    }

    PhysicsRuntime::EBodyType Engine::UShapeComponent::GetBodyType() const {
        return mBodyType;
    }

    bool Engine::UShapeComponent::SetBodyType(PhysicsRuntime::EBodyType BodyType) {
        if (BodyType != PhysicsRuntime::EBodyType::Static && BodyType != PhysicsRuntime::EBodyType::Kinematic && BodyType != PhysicsRuntime::EBodyType::Dynamic) {
            return false;
        }

        if (mPhysicsEnabled && HasBegunPlay() && (mBodyInstance == nullptr || mBodyType != BodyType)) {
            auto Body{CreatePhysicsBody(BodyType)};

            if (Body == nullptr) {
                return false;
            }

            mBodyInstance = std::move(Body);
        }

        mBodyType = BodyType;

        return true;
    }

    void Engine::UShapeComponent::Serialize(Core::FArchive& Archive) {
        Super::Serialize(Archive);

        if (Archive.HasError() || !mPhysicalMaterialPath.empty() || (Archive.IsLoading() && !Archive.HasField("Restitution"))) {
            return;
        }

        float Restitution{mLegacyPhysicalMaterial != nullptr ? mLegacyPhysicalMaterial->GetRestitution() : 0.0f};
        Archive.Serialize("Restitution", Restitution);

        if (!Archive.IsLoading() || Archive.HasError()) {
            return;
        }

        if (Restitution == 0.0f) {
            mLegacyPhysicalMaterial.reset();
            return;
        }

        auto Legacy{CoreUObject::NewObject<PhysicsRuntime::UPhysicalMaterial>()};

        if (!Legacy->SetRestitution(Restitution)) {
            Archive.SetError("Invalid legacy restitution");
            return;
        }

        mLegacyPhysicalMaterial = std::move(Legacy);
    }

    const Core::FString& Engine::UShapeComponent::GetPhysicalMaterialPath() const {
        return mPhysicalMaterialPath;
    }

    bool Engine::UShapeComponent::SetPhysicalMaterialPath(const Core::FString& Path) {
        if (Path.find('\0') != Core::FString::npos) {
            return false;
        }

        const std::filesystem::path AssetPath{Core::FilePath::FromUtf8(Path)};

        if (!Path.empty() && (AssetPath.has_root_path() || AssetPath.extension() != L".masset")) {
            return false;
        }

        for (const auto& Part : AssetPath) {
            if (Part == L"..") {
                return false;
            }
        }

        const Engine::UWorld* World{GetWorld()};
        Asset::FAssetManager* Assets{World != nullptr ? World->GetAssets() : nullptr};
        const PhysicsRuntime::UPhysicalMaterial* Material{&GetDefaultPhysicalMaterial()};

        if (!Path.empty() && Assets != nullptr) {
            Core::FString Error{};
            Material = Assets->LoadPhysicalMaterial(AssetPath, Error);

            if (Material == nullptr) {
                return false;
            }
        }

        if (HasPhysicsBody()) {
            PhysicsRuntime::FPhysicsScene* Scene{World != nullptr ? World->GetPhysicsScene() : nullptr};

            if (Scene == nullptr || (!Path.empty() && Assets == nullptr) || !Scene->SetBodyMaterial(*mBodyInstance, *Material)) {
                return false;
            }
        }

        mPhysicalMaterialPath = Path;
        mLegacyPhysicalMaterial.reset();

        return true;
    }

    const PhysicsRuntime::UPhysicalMaterial& Engine::UShapeComponent::GetPhysicalMaterial() const {
        const Engine::UWorld* World{GetWorld()};
        Asset::FAssetManager* Assets{World != nullptr ? World->GetAssets() : nullptr};

        if (!mPhysicalMaterialPath.empty() && Assets != nullptr) {
            Core::FString Error{};

            if (const auto* Material{Assets->LoadPhysicalMaterial(Core::FilePath::FromUtf8(mPhysicalMaterialPath), Error)}) {
                return *Material;
            }

            MACAW_LOG(LogShapePhysics, Warning, "Could not load physical material '{}': {}", mPhysicalMaterialPath, Error);
        }

        return mLegacyPhysicalMaterial != nullptr ? *mLegacyPhysicalMaterial : GetDefaultPhysicalMaterial();
    }

    const PhysicsRuntime::UBodySetup& Engine::UShapeComponent::GetBodySetup() {
        if (mBodySetup == nullptr) {
            mBodySetup = CoreUObject::NewObject<PhysicsRuntime::UBodySetup>();
        }

        PhysicsRuntime::FAggregateGeom& AggGeom{mBodySetup->GetAggGeom()};
        AggGeom.Clear();

        const Engine::FCollisionShape Shape{GetCollisionShape()};

        if (Shape.IsBox()) {
            AggGeom.mBoxElems.push_back(PhysicsRuntime::FBoxElem{FVector{}, FQuat{}, Shape.GetBoxHalfExtent() * 2.0f});
        } else if (Shape.IsSphere()) {
            AggGeom.mSphereElems.push_back(PhysicsRuntime::FSphereElem{FVector{}, Shape.GetRadius()});
        } else if (Shape.IsCapsule()) {
            AggGeom.mCapsuleElems.push_back(PhysicsRuntime::FCapsuleElem{FVector{}, FQuat{}, Shape.GetRadius(), Shape.GetCapsuleAxisHalfLength() * 2.0f});
        }

        return *mBodySetup;
    }

    bool Engine::UShapeComponent::HasPhysicsBody() const {
        return mBodyInstance != nullptr;
    }

    void Engine::UShapeComponent::ReleasePhysicsBody() {
        mBodyInstance.reset();
    }

    void Engine::UShapeComponent::PushKinematicTarget(float Alpha) {
        if (mBodyType != PhysicsRuntime::EBodyType::Kinematic || !HasPhysicsBody()) {
            return;
        }

        physx::PxRigidDynamic* Body{mBodyInstance->GetActor()->is<physx::PxRigidDynamic>()};
        const physx::PxTransform Target{MakePhysicsPose()};
        const physx::PxTransform Current{Body->getGlobalPose()};

        if (Target.p == Current.p && Target.q == Current.q) {
            return;
        }

        if (Alpha == 1.0f) {
            Body->setKinematicTarget(Target);
            return;
        }

        const FQuat Rotation{FQuat{Current.q.x, Current.q.y, Current.q.z, Current.q.w}.Slerp(GetTransform().mRotation, Alpha)};
        const physx::PxTransform Next{Current.p + (Target.p - Current.p) * Alpha, physx::PxQuat{Rotation.x, Rotation.y, Rotation.z, Rotation.w}};
        Body->setKinematicTarget(Next);
    }

    void Engine::UShapeComponent::PullPhysicsTransform(bool MoveOwner) {
        if (!HasPhysicsBody() || mBodyType == PhysicsRuntime::EBodyType::Kinematic) {
            return;
        }

        const physx::PxTransform Pose{mBodyInstance->GetActor()->getGlobalPose()};
        const FTransform Previous{GetTransform()};
        const FTransform Transform{FVector{Pose.p.x, Pose.p.y, Pose.p.z}, FQuat{Pose.q.x, Pose.q.y, Pose.q.z, Pose.q.w}.Normalized(), Previous.mScale};

        if (Transform.mLocation == Previous.mLocation && Transform.mRotation == Previous.mRotation) {
            return;
        }

        Engine::AActor* Owner{GetOwner()};

        if (MoveOwner && Owner != nullptr && Owner->GetRootComponent() != nullptr) {
            if (Owner->GetRootComponent() == this) {
                Owner->SetActorTransform(Transform);
                return;
            }

            const FQuat DeltaRotation{(Transform.mRotation * Previous.mRotation.Inverse()).Normalized()};
            FTransform OwnerTransform{Owner->GetActorTransform()};
            OwnerTransform.mLocation = Transform.mLocation + DeltaRotation.RotateVector(OwnerTransform.mLocation - Previous.mLocation);
            OwnerTransform.mRotation = (DeltaRotation * OwnerTransform.mRotation).Normalized();
            Owner->SetActorTransform(OwnerTransform);
        }

        SetTransform(Transform);
    }

    const PhysicsRuntime::UPhysicalMaterial& Engine::UShapeComponent::GetDefaultPhysicalMaterial() const {
        const PhysicsRuntime::UPhysicalMaterial* Material{mBodySetup != nullptr ? mBodySetup->GetPhysMaterial().Get() : nullptr};
        return Material != nullptr ? *Material : PhysicsRuntime::UPhysicalMaterial::GetDefault();
    }

    physx::PxTransform Engine::UShapeComponent::MakePhysicsPose() const {
        const FTransform& Transform{GetTransform()};
        const FVector& Location{Transform.mLocation};
        const FQuat& Rotation{Transform.mRotation};

        return physx::PxTransform{physx::PxVec3{Location.x, Location.y, Location.z}, physx::PxQuat{Rotation.x, Rotation.y, Rotation.z, Rotation.w}};
    }

    std::unique_ptr<PhysicsRuntime::FBodyInstance> Engine::UShapeComponent::CreatePhysicsBody(PhysicsRuntime::EBodyType BodyType) {
        const Engine::UWorld* World{GetWorld()};
        PhysicsRuntime::FPhysicsScene* Scene{World != nullptr ? World->GetPhysicsScene() : nullptr};

        if (Scene == nullptr) {
            return {};
        }

        const physx::PxTransform Pose{MakePhysicsPose()};
        const PhysicsRuntime::UBodySetup& Setup{GetBodySetup()};
        const PhysicsRuntime::UPhysicalMaterial& Material{GetPhysicalMaterial()};

        return Scene->CreateBody(BodyType, Pose, Setup, Material);
    }

    /**
     * @brief 겹침 시작 델리게이트를 호출합니다.
     */
    void Engine::UShapeComponent::OnBeginOverlap(Engine::UShapeComponent* Other) {
        auto* OtherActor{Other != nullptr ? Other->GetOwner() : nullptr};
        mOnComponentBeginOverlap.Broadcast(this, OtherActor, Other);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::BeginOverlap, this, OtherActor, Other, nullptr);
        }
    }

    /**
     * @brief 겹침 종료 델리게이트를 호출합니다.
     */
    void Engine::UShapeComponent::OnEndOverlap(Engine::UShapeComponent* Other) {
        auto* OtherActor{Other != nullptr ? Other->GetOwner() : nullptr};
        mOnComponentEndOverlap.Broadcast(this, OtherActor, Other);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::EndOverlap, this, OtherActor, Other, nullptr);
        }
    }

    /**
     * @brief 적중 액터·컴포넌트와 충돌 정보를 델리게이트로 전달합니다.
     */
    void Engine::UShapeComponent::OnHit(Engine::AActor* OtherActor, Engine::UPrimitiveComponent* OtherComp, const Engine::FHitResult& Hit) {
        mOnComponentHit.Broadcast(this, OtherActor, OtherComp, Hit);
        if (auto* Owner{GetOwner()}; Owner != nullptr && !Owner->IsPendingDestroy()) {
            Owner->GetOnComponentCollision().Broadcast(Engine::AActor::EComponentCollision::Hit, this, OtherActor, OtherComp, &Hit);
        }
    }

    /**
     * @brief 현재 표시·에셋 상태에서 렌더 프록시를 생성할지 판정합니다.
     */
    bool Engine::UShapeComponent::ShouldCreateRenderState() const {
        return false;
    }

    /**
     * @brief 충돌 형상을 선택 강조 색상 또는 기본 색상의 선으로 수집합니다.
     */
    void Engine::UShapeComponent::DrawDebug(Engine::FPrimitiveDrawInterface& DrawInterface, bool Highlight) const {
        Engine::FCollisionShape Shape{GetCollisionShape()};
        FVector Pos{GetTransform().mLocation};
        FQuat Rot{GetTransform().mRotation};
        FVector Color{Highlight ? FVector{1.0f, 0.0f, 0.0f} : FVector{mShapeColor.x, mShapeColor.y, mShapeColor.z}};

        if (Shape.IsBox()) {
            Engine::FOrientedBox Box{Engine::Collision::ToOrientedBox(Shape, Pos, Rot)};

            FVector V0{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V1{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V2{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V3{Box.GetCenter() + Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V4{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V5{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y + Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V6{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x - Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};
            FVector V7{Box.GetCenter() - Box.GetAxis()[0] * Box.GetExtent().x + Box.GetAxis()[1] * Box.GetExtent().y - Box.GetAxis()[2] * Box.GetExtent().z};

            DrawInterface.DrawLine(V0, V1, Color);
            DrawInterface.DrawLine(V1, V2, Color);
            DrawInterface.DrawLine(V2, V3, Color);
            DrawInterface.DrawLine(V3, V0, Color);
            DrawInterface.DrawLine(V4, V5, Color);
            DrawInterface.DrawLine(V5, V6, Color);
            DrawInterface.DrawLine(V6, V7, Color);
            DrawInterface.DrawLine(V7, V4, Color);
            DrawInterface.DrawLine(V0, V4, Color);
            DrawInterface.DrawLine(V1, V5, Color);
            DrawInterface.DrawLine(V2, V6, Color);
            DrawInterface.DrawLine(V3, V7, Color);
        }
        else if (Shape.IsSphere()) {
            Engine::FCapsuleSegment Sphere{Engine::Collision::ToCapsuleSegment(Shape, Pos, Rot)};

            const FVector F{Rot.ForwardVector()};
            const FVector Rt{Rot.RightVector()};
            const FVector Up{Rot.UpVector()};
            const float R{Sphere.GetRadius()};

            AddArc(DrawInterface, Sphere.GetA(), F, Rt, R, 0_rad, 1_turn, Color);
            AddArc(DrawInterface, Sphere.GetA(), F, Up, R, 0_rad, 1_turn, Color);
            AddArc(DrawInterface, Sphere.GetA(), Rt, Up, R, 0_rad, 1_turn, Color);
        }
        else if (Shape.IsCapsule()) {
            Engine::FCapsuleSegment Capsule{Engine::Collision::ToCapsuleSegment(Shape, Pos, Rot)};

            const FVector F{Rot.ForwardVector()};
            const FVector Rt{Rot.RightVector()};
            const FVector Up{Rot.UpVector()};
            const float R{Capsule.GetRadius()};

            AddArc(DrawInterface, Capsule.GetA(), F, Rt, R, 0_rad, 1_turn, Color);
            AddArc(DrawInterface, Capsule.GetB(), F, Rt, R, 0_rad, 1_turn, Color);

            DrawInterface.DrawLine(Capsule.GetA() + F * R, Capsule.GetB() + F * R, Color);
            DrawInterface.DrawLine(Capsule.GetA() - F * R, Capsule.GetB() - F * R, Color);
            DrawInterface.DrawLine(Capsule.GetA() + Rt * R, Capsule.GetB() + Rt * R, Color);
            DrawInterface.DrawLine(Capsule.GetA() - Rt * R, Capsule.GetB() - Rt * R, Color);

            AddArc(DrawInterface, Capsule.GetB(), F, Up, R, 0_rad, 0.5_turn, Color);
            AddArc(DrawInterface, Capsule.GetB(), Rt, Up, R, 0_rad, 0.5_turn, Color);
            AddArc(DrawInterface, Capsule.GetA(), F, -Up, R, 0_rad, 0.5_turn, Color);
            AddArc(DrawInterface, Capsule.GetA(), Rt, -Up, R, 0_rad, 0.5_turn, Color);
        }
    }

    /**
     * @brief 두 컴포넌트의 월드 충돌 형상이 겹치는지 검사합니다.
     */
    bool Engine::UShapeComponent::IsOverlapping(const Engine::UShapeComponent& Other) const {
        if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
            return false;
        }

        return Engine::Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation);
    }

    /**
     * @brief 두 형상의 월드 접촉 법선·깊이·위치를 계산합니다.
     */
    bool Engine::UShapeComponent::ComputeContact(const Engine::UShapeComponent& Other, Engine::FCollisionContact& OutContact) const {
        if (&Other == this || !BoundsOverlap(CalcBounds(), Other.CalcBounds())) {
            return false;
        }

        return Engine::Collision::Overlap(GetCollisionShape(), GetTransform().mLocation, GetTransform().mRotation, Other.GetCollisionShape(), Other.GetTransform().mLocation, Other.GetTransform().mRotation, &OutContact);
    }

    /**
     * @brief 선택한 경우에만 보조 충돌 도형을 그릴지 검사합니다.
     */
    bool Engine::UShapeComponent::ShouldDrawOnlyIfSelected() const {
        return mDrawOnlyIfSelected;
    }

    /**
     * @brief 충돌 형상에 적용할 최대 절대 축 스케일을 반환합니다.
     */
    float Engine::UShapeComponent::GetShapeScale() const {
        FVector Scale{GetTransform().mScale};

        return std::min({ std::abs(Scale.x), std::abs(Scale.y), std::abs(Scale.z) });
    }

    /**
     * @brief 컴포넌트 겹침 시작 이벤트를 반환합니다.
     */
    Engine::UShapeComponent::FOverlapDelegate& Engine::UShapeComponent::GetOnComponentBeginOverlap() {
        return mOnComponentBeginOverlap;
    }

    /**
     * @brief 컴포넌트 겹침 종료 이벤트를 반환합니다.
     */
    Engine::UShapeComponent::FOverlapDelegate& Engine::UShapeComponent::GetOnComponentEndOverlap() {
        return mOnComponentEndOverlap;
    }

    /**
     * @brief 컴포넌트 충돌 이벤트를 반환합니다.
     */
    Engine::UShapeComponent::FHitDelegate& Engine::UShapeComponent::GetOnComponentHit() {
        return mOnComponentHit;
    }

    bool Engine::UShapeComponent::BlocksMovement() const {
        return mBlocksMovement;
    }

    void Engine::UShapeComponent::SetBlocksMovement(bool Blocks) {
        mBlocksMovement = Blocks;
    }

    /**
     * @brief 컴포넌트 변환을 적용한 월드 충돌 경계를 계산합니다.
     */
    FBox Engine::UShapeComponent::CalcBounds() const {
        return CalcLocalBounds().Transformed(GetWorldMatrix());
    }

    /**
     * @brief 월드 공간의 축 정렬 경계 상자를 반환합니다.
     */
    FBox Engine::UShapeComponent::GetWorldBounds() const {
        return CalcBounds();
    }

    /**
     * @brief 표시할 프리미티브 메시 종류를 반환합니다.
     */
    Engine::EPrimitiveType Engine::UShapeComponent::GetPrimitiveType() const {
        return Engine::EPrimitiveType::Cube;
    }

    /**
     * @brief 충돌 도형을 표시할 RGBA 색상을 반환합니다.
     */
    const FVector4& Engine::UShapeComponent::GetShapeColor() const {
        return mShapeColor;
    }

    /**
     * @brief 충돌 도형을 표시할 RGBA 색상을 설정합니다.
     */
    bool Engine::UShapeComponent::SetShapeColor(const FVector4& Color) {
        if (!std::isfinite(Color.x) || !std::isfinite(Color.y) || !std::isfinite(Color.z) || !std::isfinite(Color.w)) {
            return false;
        }

        mShapeColor = Color;

        return true;
    }

    /**
     * @brief 선택한 경우에만 충돌 도형을 표시할지 여부를 설정합니다.
     */
    void Engine::UShapeComponent::SetDrawOnlyIfSelected(bool DrawOnlyIfSelected) {
        mDrawOnlyIfSelected = DrawOnlyIfSelected;
    }

}
