#include "FBonePreview.h"

#include <algorithm>
#include <cmath>

namespace Editor {
    Core::TArray<Editor::FBoneShape> BuildBoneShapes(const Core::TArray<Asset::FMeshImportBone>& Bones, const Core::TArray<Core::FMatrix>& Transforms, float Radius) {
        const float MaxJointRadius{Radius * 0.019f};
        const float MinJointRadius{MaxJointRadius * 0.12f};
        const float MaxBoneHalfWidth{Radius * 0.05f};
        Core::TArray<float> JointRadii{};
        JointRadii.resize(Bones.size(), MaxJointRadius);

        Core::TArray<Editor::FBoneShape> Shapes{};
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
