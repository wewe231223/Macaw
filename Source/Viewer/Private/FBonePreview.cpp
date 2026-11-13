/**
 * @file FBonePreview.cpp
 * @brief 스켈레톤의 본·관절 표시와 선택에 사용할 도형을 생성합니다.
 * @ingroup Viewer
 */

#include "Viewer/FBonePreview.h"

#include <algorithm>
#include <cmath>

namespace Viewer {
    FBoneShape::FBoneShape(Engine::EEditorMesh Mesh, const Core::FMatrix& World, Core::Uint32 Bone)
		: mMesh{Mesh},
		  mWorld{World},
		  mBone{Bone} {
    }

    Engine::EEditorMesh FBoneShape::GetMesh() const {
        return mMesh;
    }

    const Core::FMatrix& FBoneShape::GetWorld() const {
        return mWorld;
    }

    Core::Uint32 FBoneShape::GetBone() const {
        return mBone;
    }

    /**
     * @brief 스켈레톤의 전역 포즈에서 본 팔면체와 관절 구 표시 도형을 만듭니다.
     * @param Bones 부모 인덱스와 바인드 변환을 가진 본 배열입니다.
     * @param Transforms 스켈레톤 본별 전역 변환 행렬입니다.
     * @param Radius 본 표시 크기의 기준 반지름입니다.
     * @return 본 선택·표시에 사용할 도형 배열입니다.
     */
    Core::TArray<Viewer::FBoneShape> BuildBoneShapes(const Core::TArray<Asset::FMeshImportBone>& Bones, const Core::TArray<Core::FMatrix>& Transforms, float Radius) {
        const float MaxJointRadius{Radius * 0.019f};
        const float MinJointRadius{MaxJointRadius * 0.12f};
        const float MaxBoneHalfWidth{Radius * 0.05f};
        Core::TArray<float> JointRadii{};
        JointRadii.resize(Bones.size(), MaxJointRadius);

        Core::TArray<Viewer::FBoneShape> Shapes{};
        Shapes.reserve(Bones.size() * 2);

        for (Core::Uint32 Bone{}; Bone < Bones.size(); ++Bone) {
            const Core::Int32 ParentIndex{Bones[Bone].GetParentIndex()};

            if (ParentIndex < 0) {
                continue;
            }

            const Core::Uint32 Parent{static_cast<Core::Uint32>(ParentIndex)};
            const Core::FVector Start{Transforms[Parent].TransformPosition({})};
            const Core::FVector Offset{Transforms[Bone].TransformPosition({}) - Start};
            const float Length{Offset.Length()};

            if (!std::isfinite(Length) || Length <= MaxJointRadius * 0.01f) {
                continue;
            }

            const float JointRadius{std::clamp(Length * 0.052f, MinJointRadius, MaxJointRadius)};
            JointRadii[Bone] = std::min(JointRadii[Bone], JointRadius);
            JointRadii[Parent] = std::min(JointRadii[Parent], JointRadius);

            const float HalfWidth{std::min(Length * 0.105f, MaxBoneHalfWidth)};
            const Core::FVector Axis{Offset / Length};
            const Core::FVector Up{std::abs(Axis.z) < 0.8f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
            const Core::FMatrix Rotation{Core::FMatrix::LookTo({}, Axis, Up).Transposed()};
            const Core::FMatrix World{Core::FMatrix::Scale({HalfWidth, HalfWidth, Length}) * Rotation * Core::FMatrix::Translation(Start)};

            Shapes.push_back({Engine::EEditorMesh::BoneOctahedron, World, Parent});
        }

        for (Core::Uint32 Bone{}; Bone < Bones.size(); ++Bone) {
            const Core::FVector Joint{Transforms[Bone].TransformPosition({})};
            const float Diameter{JointRadii[Bone] * 2.0f};
            const Core::FMatrix World{Core::FMatrix::Scale({Diameter, Diameter, Diameter}) * Core::FMatrix::Translation(Joint)};

            Shapes.push_back({Engine::EEditorMesh::Sphere, World, Bone});
        }

        return Shapes;
    }
}
