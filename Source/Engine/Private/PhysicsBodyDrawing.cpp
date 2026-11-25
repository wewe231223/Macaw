#include "Core/Types.h"
#include "Engine/PhysicsBodyDrawing.h"
#include "Core/Literals.h"
#include "Core/Math/FTransform.h"
#include "CoreUObject/Reflection/Cast.h"
#include "Engine/AActor.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/FPrimitiveDrawInterface.h"
#include "Engine/ULevel.h"
#include "Engine/UWorld.h"
#include "PhysicsRuntime/FPhysicsScene.h"
#include "PhysicsRuntime/PhysicsConversion.h"
#include "PhysicsRuntime/UPhysicsAsset.h"

#include <algorithm>
#include <utility>

using namespace Core::Literals;

namespace Engine::PhysicsBodyDrawing {
    namespace {
        const FVector StaticColor{0.35f, 0.6f, 1.0f};
        const FVector KinematicColor{1.0f, 0.65f, 0.15f};
        const FVector DynamicColor{0.35f, 1.0f, 0.35f};

        void DrawSphere(Engine::FPrimitiveDrawInterface& Draw, const FVector& Center, const FQuat& Rotation, Core::FLength Radius, const FVector& Color, bool DepthTest, float Thickness, float Opacity) {
            const FVector Forward{Rotation.ForwardVector()};
            const FVector Right{Rotation.RightVector()};
            const FVector Up{Rotation.UpVector()};

            Draw.DrawArc(Center, Forward, Right, Radius, 0_rad, 1_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Center, Forward, Up, Radius, 0_rad, 1_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Center, Right, Up, Radius, 0_rad, 1_turn, Color, DepthTest, Thickness, Opacity);
        }

        void DrawCapsule(Engine::FPrimitiveDrawInterface& Draw, const FVector& Center, const FQuat& Rotation, Core::FLength Radius, Core::FLength HalfLength, const FVector& Color, bool DepthTest, float Thickness, float Opacity) {
            const FVector Forward{Rotation.ForwardVector()};
            const FVector Right{Rotation.RightVector()};
            const FVector Up{Rotation.UpVector()};

            // 캡슐 축은 형상 로컬 Z이며 반길이는 형상 중심에서 반구 중심까지의 거리입니다.
            const FVector Bottom{Center - Up * HalfLength.GetBase()};
            const FVector Top{Center + Up * HalfLength.GetBase()};
            const FVector ForwardOffset{Forward * Radius.GetBase()};
            const FVector RightOffset{Right * Radius.GetBase()};

            Draw.DrawArc(Bottom, Forward, Right, Radius, 0_rad, 1_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Top, Forward, Right, Radius, 0_rad, 1_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawLine(Bottom + ForwardOffset, Top + ForwardOffset, Color, DepthTest, Thickness, Opacity);
            Draw.DrawLine(Bottom - ForwardOffset, Top - ForwardOffset, Color, DepthTest, Thickness, Opacity);
            Draw.DrawLine(Bottom + RightOffset, Top + RightOffset, Color, DepthTest, Thickness, Opacity);
            Draw.DrawLine(Bottom - RightOffset, Top - RightOffset, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Top, Forward, Up, Radius, 0_rad, 0.5_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Top, Right, Up, Radius, 0_rad, 0.5_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Bottom, Forward, -Up, Radius, 0_rad, 0.5_turn, Color, DepthTest, Thickness, Opacity);
            Draw.DrawArc(Bottom, Right, -Up, Radius, 0_rad, 0.5_turn, Color, DepthTest, Thickness, Opacity);
        }

        void DrawConvex(Engine::FPrimitiveDrawInterface& Draw, const physx::PxConvexMeshGeometry& Geometry, const physx::PxTransform& Pose, const FVector& Color, float Thickness) {
            const physx::PxConvexMesh& Mesh{*Geometry.convexMesh};
            const physx::PxVec3* Vertices{Mesh.getVertices()};
            const physx::PxU8* Indices{Mesh.getIndexBuffer()};
            const auto ToWorld{[&](physx::PxU8 Index) {
                return PhysicsRuntime::ToFVector(Pose.transform(Geometry.scale.transform(Vertices[Index])));
            }};

            for (physx::PxU32 Index{}; Index < Mesh.getNbPolygons(); ++Index) {
                physx::PxHullPolygon Polygon{};

                if (!Mesh.getPolygonData(Index, Polygon)) {
                    continue;
                }

                const physx::PxU8* Face{Indices + Polygon.mIndexBase};

                for (physx::PxU16 Corner{}; Corner < Polygon.mNbVerts; ++Corner) {
                    const physx::PxU8 First{Face[Corner]};
                    const physx::PxU8 Second{Face[(Corner + 1) % Polygon.mNbVerts]};

                    if (First < Second) {
                        Draw.DrawLine(ToWorld(First), ToWorld(Second), Color, false, Thickness);
                    }
                }
            }
        }

        void DrawShape(Engine::FPrimitiveDrawInterface& Draw, const physx::PxShape& Shape, const physx::PxTransform& BodyPose, const FVector& Color, float Thickness) {
            const physx::PxGeometryHolder Geometry{Shape.getGeometry()};
            const physx::PxTransform Pose{BodyPose * Shape.getLocalPose()};
            const FVector Center{PhysicsRuntime::ToFVector(Pose.p)};
            const FQuat Rotation{PhysicsRuntime::ToFQuat(Pose.q)};

            switch (Geometry.getType()) {
            case physx::PxGeometryType::eSPHERE:
                DrawSphere(Draw, Center, Rotation, Core::FLength::FromBase(Geometry.sphere().radius), Color, false, Thickness, 1.0f);
                break;

            case physx::PxGeometryType::eBOX:
                Draw.DrawOrientedBox(Center, Rotation, PhysicsRuntime::ToFVector(Geometry.box().halfExtents), Color, false, Thickness);
                break;

            case physx::PxGeometryType::eCAPSULE: {
                const physx::PxCapsuleGeometry& Capsule{Geometry.capsule()};
                const FQuat CapsuleRotation{PhysicsRuntime::ToFQuat(Pose.q * PhysicsRuntime::GetSphylBasis().getConjugate())};

                DrawCapsule(Draw, Center, CapsuleRotation, Core::FLength::FromBase(Capsule.radius), Core::FLength::FromBase(Capsule.halfHeight), Color, false, Thickness, 1.0f);
                break;
            }

            case physx::PxGeometryType::eCONVEXMESH:
                DrawConvex(Draw, Geometry.convexMesh(), Pose, Color, Thickness);
                break;

            default:
                break;
            }
        }

        void DrawSceneBodies(Engine::FPrimitiveDrawInterface& Draw, const physx::PxScene& Scene, float Thickness) {
            const physx::PxActorTypeFlags Types{physx::PxActorTypeFlag::eRIGID_STATIC | physx::PxActorTypeFlag::eRIGID_DYNAMIC};
            Core::TArray<physx::PxActor*> Actors{};
            Core::TArray<physx::PxShape*> Shapes{};
            Actors.resize(Scene.getNbActors(Types));

            if (Actors.empty()) {
                return;
            }

            Actors.resize(Scene.getActors(Types, Actors.data(), static_cast<physx::PxU32>(Actors.size())));

            for (const physx::PxActor* Actor : Actors) {
                const auto* Body{static_cast<const physx::PxRigidActor*>(Actor)};
                const auto* Dynamic{Actor->is<physx::PxRigidDynamic>()};
                const FVector& Color{Dynamic == nullptr ? StaticColor : Dynamic->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC) ? KinematicColor : DynamicColor};
                const physx::PxTransform Pose{Body->getGlobalPose()};
                Shapes.resize(Body->getNbShapes());

                if (Shapes.empty()) {
                    continue;
                }

                Shapes.resize(Body->getShapes(Shapes.data(), static_cast<physx::PxU32>(Shapes.size())));

                for (const physx::PxShape* Shape : Shapes) {
                    DrawShape(Draw, *Shape, Pose, Color, Thickness);
                }
            }
        }

        void DrawComponentBodies(Engine::FPrimitiveDrawInterface& Draw, const Engine::UPrimitiveComponent& Component, float Thickness) {
            const auto* Mesh{CoreUObject::Cast<Engine::USkeletalMeshComponent>(&Component)};
            const PhysicsRuntime::UPhysicsAsset* Asset{Mesh != nullptr ? Mesh->GetPhysicsAsset() : nullptr};

            if (Asset != nullptr) {
                for (const auto& Setup : Asset->mSkeletalBodySetups) {
                    if (Setup == nullptr) {
                        continue;
                    }

                    const Index_t BoneIndex{Mesh->GetBoneIndex(Setup->GetBoneName())};
                    FTransform BoneWorld{};

                    if (BoneIndex != InvalidIndex && Mesh->GetBoneWorldTransform(BoneIndex, BoneWorld)) {
                        DrawBodySetup(Draw, *Setup, BoneWorld, KinematicColor, false, Thickness);
                    }
                }

                return;
            }

            if (const PhysicsRuntime::UBodySetup* Setup{Component.GetBodySetup()}) {
                const FVector& Color{Component.GetBodyInstance().ShouldSimulatePhysics() ? DynamicColor : StaticColor};
                DrawBodySetup(Draw, *Setup, Component.GetTransform(), Color, false, Thickness);
            }
        }
    }

    void DrawBodySetup(Engine::FPrimitiveDrawInterface& Draw, const PhysicsRuntime::UBodySetup& Setup, const FTransform& Transform, const FVector& Color, bool DepthTest, float Thickness, float Opacity) {
        const PhysicsRuntime::FKAggregateGeom& Geometry{Setup.GetAggGeom()};

        // 스케일을 반영한 바디 로컬 위치를 바디의 위치·회전으로 월드 공간에 옮깁니다.
        const auto ToWorld{[&Transform](const FVector& Local) {
            return Transform.mLocation + Transform.mRotation.RotateVector(Local);
        }};

        for (const PhysicsRuntime::FKSphereElem& Element : Geometry.mSphereElems) {
            const PhysicsRuntime::FKSphereElem Scaled{Element.GetScaled(Transform.mScale)};

            DrawSphere(Draw, ToWorld(Scaled.mCenter), Transform.mRotation, Scaled.mRadius, Color, DepthTest, Thickness, Opacity);
        }

        for (const PhysicsRuntime::FKBoxElem& Element : Geometry.mBoxElems) {
            const PhysicsRuntime::FKBoxElem Scaled{Element.GetScaled(Transform.mScale)};

            Draw.DrawOrientedBox(ToWorld(Scaled.mCenter), Transform.mRotation * Scaled.mRotation.Quaternion(), Scaled.mHalfExtent, Color, DepthTest, Thickness, Opacity);
        }

        for (const PhysicsRuntime::FKSphylElem& Element : Geometry.mSphylElems) {
            const PhysicsRuntime::FKSphylElem Scaled{Element.GetScaled(Transform.mScale)};

            DrawCapsule(Draw, ToWorld(Scaled.mCenter), Transform.mRotation * Scaled.mRotation.Quaternion(), Scaled.mRadius, Scaled.mLength * 0.5f, Color, DepthTest, Thickness, Opacity);
        }

        for (const PhysicsRuntime::FKConvexElem& Element : Geometry.mConvexElems) {
            const PhysicsRuntime::FKConvexElem Scaled{Element.GetScaled(Transform.mScale)};

            if (!Scaled.IsValid()) {
                continue;
            }

            // 이웃한 삼각형이 공유하는 모서리는 한 번만 그립니다.
            Core::TArray<std::pair<Uint32, Uint32>> Edges{};
            Edges.reserve(Scaled.mIndices.size());

            for (Index_t Triangle{}; Triangle + 2 < Scaled.mIndices.size(); Triangle += 3) {
                for (Index_t Corner{}; Corner < 3; ++Corner) {
                    const Uint32 First{Scaled.mIndices[Triangle + Corner]};
                    const Uint32 Second{Scaled.mIndices[Triangle + (Corner + 1) % 3]};

                    Edges.emplace_back(std::min(First, Second), std::max(First, Second));
                }
            }

            std::sort(Edges.begin(), Edges.end());
            Edges.erase(std::unique(Edges.begin(), Edges.end()), Edges.end());

            for (const auto& [First, Second] : Edges) {
                Draw.DrawLine(ToWorld(Scaled.mVertices[First]), ToWorld(Scaled.mVertices[Second]), Color, DepthTest, Thickness, Opacity);
            }
        }
    }

    void DrawWorldBodies(Engine::FPrimitiveDrawInterface& Draw, const Engine::UWorld& World, float Thickness) {
        if (const PhysicsRuntime::FPhysicsScene* Physics{World.GetPhysicsScene()}; Physics != nullptr && Physics->GetScene() != nullptr) {
            DrawSceneBodies(Draw, *Physics->GetScene(), Thickness);

            return;
        }

        const Engine::ULevel* Level{World.GetPersistentLevel()};

        if (Level == nullptr) {
            return;
        }

        for (const auto& Actor : Level->GetActors()) {
            if (Actor->IsPendingDestroy()) {
                continue;
            }

            for (const auto& Component : Actor->GetComponents()) {
                const auto* Primitive{CoreUObject::Cast<Engine::UPrimitiveComponent>(Component.get())};

                if (Primitive != nullptr && Primitive->IsRegistered() && Primitive->ShouldCreatePhysicsState()) {
                    DrawComponentBodies(Draw, *Primitive, Thickness);
                }
            }
        }
    }
}
