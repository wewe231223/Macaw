/**
 * @file FAssimpMeshImporter.cpp
 * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Asset
 */

#include "Asset/Import/FAssimpMeshImporter.h"
#include "Asset/Import/FMeshImportResult.h"
#include "Core/FilePath.h"
#include "FMeshImportIOSystem.h"
#include "FMeshImportTransform.h"

#include <algorithm>
#include <array>
#include <assimp/Importer.hpp>
#include <assimp/config.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <d3d11.h>
#include <limits>
#include <utility>

namespace Asset {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {

        /**
         * @brief 임포터 벡터의 XYZ 성분을 엔진 벡터로 옮깁니다.
         * @param[in] Vector 엔진 형식으로 옮길 임포터 벡터입니다.
         * @return XYZ 성분을 옮긴 엔진 벡터입니다.
         */
        Core::FVector ToVector(const aiVector3D& Vector) {
            return {Vector.x, Vector.y, Vector.z};
        }

        /**
         * @brief 임포터 행렬 성분을 엔진 행렬 저장 형식으로 옮깁니다.
         * @param[in] Matrix 엔진 형식으로 옮길 임포터 행렬입니다.
         * @return 성분을 옮긴 엔진 행렬입니다.
         */
        Core::FMatrix ToMatrix(const aiMatrix4x4& Matrix) {
            // Assimp 행렬은 열 벡터 규약(이동이 4번째 열)이므로 전치해서 행 벡터 규약의 행렬로 만듭니다.
            Core::FMatrix Result{};

            for (Core::Int32 Row{}; Row < 4; ++Row) {
                for (Core::Int32 Column{}; Column < 4; ++Column) {
                    Result(Row, Column) = Matrix[static_cast<unsigned int>(Column)][static_cast<unsigned int>(Row)];
                }
            }

            return Result;
        }

    }

    /**
     * @brief 가져올 원본 파일 경로와 임포트 옵션을 보관하도록 Assimp 임포터를 초기화합니다.
     */
    Asset::FAssimpMeshImporter::FAssimpMeshImporter(std::filesystem::path Source, const Asset::FAssetImportOption& Option)
		: IMeshImporter{std::move(Source), Option} {
    }

    /**
     * @brief Assimp 임포터와 읽어 들인 장면을 해제합니다.
     */
    Asset::FAssimpMeshImporter::~FAssimpMeshImporter() = default;

    /**
     * @brief Assimp 재질에서 우선 종류의 텍스처를 찾고 없으면 대체 종류의 텍스처 참조를 읽습니다.
     */
    Asset::FMeshImportTexture Asset::FAssimpMeshImporter::ReadTexture(const aiMaterial& Material, aiTextureType PrimaryType, aiTextureType FallbackType) const {
        aiString Texture{};

        if (Material.GetTexture(PrimaryType, 0, &Texture) != AI_SUCCESS && Material.GetTexture(FallbackType, 0, &Texture) != AI_SUCCESS) {
            return {};
        }

        Core::FString Reference{Texture.C_Str()};

        std::replace(Reference.begin(), Reference.end(), '\\', '/');

        if (Reference.starts_with("*") || mScene->GetEmbeddedTexture(Texture.C_Str()) != nullptr) {
            return Asset::FMeshImportTexture::FromEmbedded(std::move(Reference));
        }

        return Asset::FMeshImportTexture{(GetSourceDirectory() / Core::FilePath::FromUtf8(Reference)).lexically_normal()};
    }

    /**
     * @brief Assimp 메시의 정점과 삼각형을 엔진 공간으로 변환해 대상 메시에 추가하고 서브메시를 구성합니다.
     */
    bool Asset::FAssimpMeshImporter::ReadMesh(const aiMesh& Source, const Asset::FMeshImportTransform& Transform, Asset::FMeshImportMesh& Target) {
        Asset::FMeshData& MeshData{Target.GetMeshData()};
        Core::FString& Error{GetError()};

        constexpr std::size_t MaximumBytes{std::numeric_limits<Core::Uint32>::max()};
        const std::size_t MaximumVertices{MaximumBytes / sizeof(std::array<float, 7>)};
        constexpr std::size_t MaximumIndices{MaximumBytes / sizeof(Core::Uint32)};
        Core::TArray<Core::FVector>& Positions{MeshData.GetPositions()};
        Core::TArray<Core::FVector>& Normals{MeshData.GetNormals()};
        Core::TArray<Core::FVector4>& Tangents{MeshData.GetTangents()};
        Core::TArray<Core::TArray<Core::FVector2>>& UVChannels{MeshData.GetUVChannels()};
        Core::TArray<Core::FVector4>& Colors{MeshData.GetColors()};
        Core::TArray<Core::Uint32>& Indices{MeshData.GetIndices()};
        const std::size_t VertexCount{Positions.size()};

        if (!Source.HasPositions() || !Source.HasFaces() || Source.mNumVertices > MaximumVertices - VertexCount || Source.mNumFaces > (MaximumIndices - Indices.size()) / 3 || Source.mMaterialIndex >= mScene->mNumMaterials) {
            Error = "The source contains an empty mesh or exceeds mesh buffer limits";

            return false;
        }

        // UV 채널 수는 메시마다 다를 수 있으므로, 지금까지의 채널 수와 이 메시가 사용하는 가장 높은 채널 번호 + 1 중 큰 값으로 맞춥니다.
        std::size_t UVChannelCount{UVChannels.size()};

        for (Core::Uint32 Channel{}; Channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++Channel) {
            if (Source.HasTextureCoords(Channel)) {
                UVChannelCount = std::max<std::size_t>(UVChannelCount, Channel + 1);
            }
        }

        const std::size_t UVStride{std::max<std::size_t>(1, UVChannelCount) * sizeof(float) * 2};

        if (UVStride > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES || VertexCount + Source.mNumVertices > MaximumBytes / UVStride) {
            Error = "The source exceeds mesh buffer limits";

            return false;
        }

        // 새로 생긴 채널은 이전 메시 정점 몫을 (0, 0)으로 채워 모든 채널의 길이를 정점 수와 맞춥니다.
        UVChannels.resize(UVChannelCount);

        for (Core::TArray<Core::FVector2>& Channel : UVChannels) {
            Channel.resize(VertexCount);
            Channel.reserve(VertexCount + Source.mNumVertices);
        }

        Positions.reserve(VertexCount + Source.mNumVertices);
        Normals.reserve(VertexCount + Source.mNumVertices);
        Tangents.reserve(VertexCount + Source.mNumVertices);
        Colors.reserve(VertexCount + Source.mNumVertices);
        Indices.reserve(Indices.size() + static_cast<std::size_t>(Source.mNumFaces) * 3);

        const Core::Uint32 BaseVertex{static_cast<Core::Uint32>(VertexCount)};
        const Core::Uint32 FirstIndex{static_cast<Core::Uint32>(Indices.size())};
        const Core::Uint32 Slot{Target.FindOrAddMaterialSlot(Source.mMaterialIndex, GetResult().GetMaterials()[Source.mMaterialIndex].GetName())};

        for (Core::Uint32 VertexIndex{}; VertexIndex < Source.mNumVertices; ++VertexIndex) {
            const Core::FVector Position{Transform.TransformPosition(ToVector(Source.mVertices[VertexIndex]))};
            Core::FVector Normal{Transform.TransformNormal(Source.HasNormals() ? ToVector(Source.mNormals[VertexIndex]) : Core::FVector{0.0f, 1.0f, 0.0f})};

            if (!Position.IsFinite() || !Normal.Normalize()) {
                Error = "The source contains invalid vertex attributes";

                return false;
            }

            // Assimp의 CalcTangentSpace 바이탄젠트는 원본 UV의 -V 방향이고 FBX SDK는 +V 방향입니다.
            // MakeTangent는 +V 방향을 받으므로 부호를 뒤집어 두 임포터가 같은 탄젠트 부호를 만들도록 합니다.
            const bool HasTangents{Source.HasTangentsAndBitangents()};
            const Core::FVector SourceTangent{HasTangents ? ToVector(Source.mTangents[VertexIndex]) : Core::FVector{}};
            const Core::FVector SourceBitangent{HasTangents ? ToVector(Source.mBitangents[VertexIndex]) * -1.0f : Core::FVector{}};
            const Core::FVector4 Tangent{Transform.MakeTangent(Normal, SourceTangent, SourceBitangent, 1.0f)};
            Core::FVector4 Color{1.0f, 1.0f, 1.0f, 1.0f};

            if (Source.HasVertexColors(0)) {
                const aiColor4D& SourceColor{Source.mColors[0][VertexIndex]};

                Color = {SourceColor.r, SourceColor.g, SourceColor.b, SourceColor.a};
            }

            if (!Tangent.IsFinite() || !Color.IsFinite()) {
                Error = "The source contains invalid vertex attributes";

                return false;
            }

            Positions.push_back(Position);
            Normals.push_back(Normal);
            Tangents.push_back(Tangent);
            Colors.push_back(Color);

            for (std::size_t Channel{}; Channel < UVChannelCount; ++Channel) {
                // Assimp UV는 왼쪽 아래가 원점이고 D3D 텍스처 좌표는 왼쪽 위가 원점이므로 V를 1 - V로 뒤집습니다.
                // 이 메시에 없는 채널은 (0, 0)을 넣습니다.
                const bool HasChannel{Channel < AI_MAX_NUMBER_OF_TEXTURECOORDS && Source.HasTextureCoords(static_cast<Core::Uint32>(Channel))};
                const Core::FVector2 UV{HasChannel ? Core::FVector2{Source.mTextureCoords[Channel][VertexIndex].x, 1.0f - Source.mTextureCoords[Channel][VertexIndex].y} : Core::FVector2{}};

                if (!UV.IsFinite()) {
                    Error = "The source contains invalid texture coordinates";

                    return false;
                }

                UVChannels[Channel].push_back(UV);
            }
        }

        for (Core::Uint32 FaceIndex{}; FaceIndex < Source.mNumFaces; ++FaceIndex) {
            const aiFace& Face{Source.mFaces[FaceIndex]};

            if (Face.mNumIndices != 3) {
                Error = "The source contains a face that could not be triangulated";

                return false;
            }

            for (Core::Uint32 Corner{}; Corner < 3; ++Corner) {
                // 원본의 CCW 앞면은 손 방향이 바뀌는 변환 뒤에도 화면에서 CCW로 보이므로, 엔진의 CW 앞면에 맞게 꼭짓점을 역순으로 읽습니다.
                const Core::Uint32 SourceCorner{Transform.ReversesWinding() ? 2 - Corner : Corner};

                if (Face.mIndices[SourceCorner] >= Source.mNumVertices) {
                    Error = "The source contains an invalid vertex index";

                    return false;
                }

                Indices.push_back(BaseVertex + Face.mIndices[SourceCorner]);
            }
        }

        MeshData.GetSubMeshes().emplace_back(FirstIndex, static_cast<Core::Uint32>(Source.mNumFaces * 3), Slot);

        // 스켈레톤을 읽었고 메시에 본이 있으면 정점별로 (결과 본 인덱스, 가중치)를 모아 정리합니다.
        if (GetOption().mImportSkeleton && Source.HasBones()) {
            Core::TArray<Core::TArray<std::pair<Core::Uint32, float>>> Influences(Source.mNumVertices);

            for (Core::Uint32 BoneIndex{}; BoneIndex < Source.mNumBones; ++BoneIndex) {
                const aiBone& Bone{*Source.mBones[BoneIndex]};
                const auto Found{mBoneIndices.find(Core::FString{Bone.mName.C_Str()})};

                for (Core::Uint32 WeightIndex{}; Found != mBoneIndices.end() && WeightIndex < Bone.mNumWeights; ++WeightIndex) {
                    const aiVertexWeight& Weight{Bone.mWeights[WeightIndex]};

                    if (Weight.mVertexId < Source.mNumVertices) {
                        Influences[Weight.mVertexId].emplace_back(static_cast<Core::Uint32>(Found->second), Weight.mWeight);
                    }
                }
            }

            Core::TArray<Asset::FMeshImportSkinWeight> Weights{};
            Weights.reserve(Influences.size());

            for (auto& Vertex : Influences) {
                Weights.push_back(Asset::FMeshImportSkinWeight::FromInfluences(std::move(Vertex)));
            }

            Target.AddSkinWeights(VertexCount, std::move(Weights));
        }

        return true;
    }

    /**
     * @brief 노드와 모든 자식 노드를 깊이 우선으로 순회하며 노드의 메시를 읽습니다.
     */
    bool Asset::FAssimpMeshImporter::ReadNode(const aiNode& Node, const Core::FMatrix& ParentTransform) {
        // 행 벡터 규약이므로 노드 로컬 변환을 먼저, 부모 전역 변환을 나중에 적용합니다.
        const Core::FMatrix Global{ToMatrix(Node.mTransformation) * ParentTransform};

        if (Node.mNumMeshes > 0) {
            auto& Meshes{GetResult().GetMeshes()};

            // 노드 전역 변환 뒤에 축 변환과 임포트 이동·회전·스케일(mBasis)을 적용합니다.
            const Asset::FMeshImportTransform Transform{Global * mBasis};

            if (!Transform.IsValid()) {
                GetError() = "The source mesh has a non-invertible node transform";

                return false;
            }

            // 메시를 병합하지 않으면 메시를 가진 노드마다 새 결과 메시를 만듭니다. Assimp는 재질별로 메시를 나누므로 한 노드의 메시들은 하나로 모읍니다.
            if (!GetOption().mCombineMeshes) {
                Meshes.emplace_back(Node.mName.length == 0 ? "Mesh_" + std::to_string(Meshes.size()) : Core::FString{Node.mName.C_Str()});
            }

            for (Core::Uint32 Index{}; Index < Node.mNumMeshes; ++Index) {
                if (Node.mMeshes[Index] >= mScene->mNumMeshes) {
                    GetError() = "The source node references an invalid mesh";

                    return false;
                }

                if (!ReadMesh(*mScene->mMeshes[Node.mMeshes[Index]], Transform, Meshes.back())) {
                    return false;
                }
            }
        }

        for (Core::Uint32 Index{}; Index < Node.mNumChildren; ++Index) {
            if (!ReadNode(*Node.mChildren[Index], Global)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 메시 노드를 순회하며 메시 본의 오프셋 행렬로 본 이름별 바인드 시점 전역 변환을 모읍니다.
     */
    bool Asset::FAssimpMeshImporter::CollectBindPoses(const aiNode& Node, const Core::FMatrix& ParentTransform) {
        const Core::FMatrix Global{ToMatrix(Node.mTransformation) * ParentTransform};

        for (Core::Uint32 Index{}; Index < Node.mNumMeshes; ++Index) {
            if (Node.mMeshes[Index] >= mScene->mNumMeshes) {
                continue;
            }

            const aiMesh& Mesh{*mScene->mMeshes[Node.mMeshes[Index]]};

            for (Core::Uint32 BoneIndex{}; BoneIndex < Mesh.mNumBones; ++BoneIndex) {
                const aiBone& Bone{*Mesh.mBones[BoneIndex]};
                const Core::FString Name{Bone.mName.C_Str()};
                Core::FMatrix InverseOffset{};

                // 같은 본을 여러 메시가 쓰면 처음 만난 메시의 바인드 포즈를 사용합니다.
                if (mBindPoses.contains(Name)) {
                    continue;
                }

                // 오프셋 행렬은 메시 공간을 바인드 시점의 본 공간으로 옮깁니다(Offset × BoneGlobal = MeshGlobal, 행 벡터 규약).
                // 따라서 바인드 시점의 본 전역 변환은 Offset⁻¹ × MeshGlobal입니다.
                if (!ToMatrix(Bone.mOffsetMatrix).TryInverse(InverseOffset)) {
                    GetError() = "The source bone has a non-invertible offset matrix: " + Name;

                    return false;
                }

                mBindPoses.emplace(Name, InverseOffset * Global);
            }
        }

        for (Core::Uint32 Child{}; Child < Node.mNumChildren; ++Child) {
            if (!CollectBindPoses(*Node.mChildren[Child], Global)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief 메시가 참조하는 본 이름과 같은 노드를 깊이 우선으로 순회하며 결과 본 목록에 추가합니다.
     */
    bool Asset::FAssimpMeshImporter::ReadSkeleton(const aiNode& Node, Core::Int32 ParentIndex, const Core::FMatrix& ParentBoneGlobal) {
        const Core::FString Name{Node.mName.C_Str()};
        Core::Int32 Index{ParentIndex};
        Core::FMatrix BoneGlobal{ParentBoneGlobal};

        // 메시의 본이 참조하는 노드만 본으로 추가합니다. 사이에 끼인 일반 노드는 건너뛰고 가장 가까운 조상 본을 부모로 연결합니다.
        if (const auto BindPose{mBindPoses.find(Name)}; BindPose != mBindPoses.end()) {
            auto& Bones{GetResult().GetBones()};
            Core::FMatrix ParentInverse{};

            if (!Asset::FMeshImportTransform::ConvertTransform(mBasis, BindPose->second, BoneGlobal) || !ParentBoneGlobal.TryInverse(ParentInverse)) {
                GetError() = "The source skeleton cannot be converted to the engine space";

                return false;
            }

            // mTransformation은 직계 부모 노드 기준이라 사이에 일반 노드가 끼면 부모 본 기준과 달라지므로,
            // 전역 변환에서 부모 본의 전역 변환을 되돌려 부모 본 공간 기준의 로컬 변환을 구합니다. 루트 본은 장면 원점 기준입니다.
            Index = static_cast<Core::Int32>(Bones.size());

            mBoneIndices.emplace(Name, Index);
            Bones.emplace_back(Name, ParentIndex, BoneGlobal * ParentInverse);
        }

        for (Core::Uint32 Child{}; Child < Node.mNumChildren; ++Child) {
            if (!ReadSkeleton(*Node.mChildren[Child], Index, BoneGlobal)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Assimp로 원본 메시 파일을 읽어 정점·삼각형·재질 슬롯과 텍스처 참조를 수집합니다.
     */
    bool Asset::FAssimpMeshImporter::ImportSource() {
        Core::FString& Error{GetError()};
        const Asset::FAssetImportOption& Option{GetOption()};

        mImporter.SetIOHandler(new Asset::FMeshImportIOSystem{});
        mImporter.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        mImporter.SetPropertyBool(AI_CONFIG_PP_FID_IGNORE_TEXTURECOORDS, true);

        // 좌표축 변환·UV 반전·감김 순서는 FBX 임포터와 같은 결과가 나오도록 ReadMesh에서 직접 처리하므로 ConvertToLeftHanded를 쓰지 않습니다.
        // 노드 계층도 직접 순회해야 메시 분리와 스켈레톤 옵션을 따를 수 있으므로 PreTransformVertices를 쓰지 않습니다.
        Core::Uint32 ImportFlags{aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_SortByPType | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure};

        // GenSmoothNormals와 CalcTangentSpace는 없는 요소만 만들므로, 다시 계산할 요소는 RemoveComponent로 먼저 지웁니다.
        const int RemovedComponents{Option.mNormalImportMethod == Asset::ENormalImportMethod::ComputeNormals ? aiComponent_NORMALS | aiComponent_TANGENTS_AND_BITANGENTS : Option.mNormalImportMethod == Asset::ENormalImportMethod::ImportNormals ? aiComponent_TANGENTS_AND_BITANGENTS : 0};

        if (RemovedComponents != 0) {
            mImporter.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, RemovedComponents);

            ImportFlags |= aiProcess_RemoveComponent;
        }

        mScene = mImporter.ReadFile(Core::FilePath::ToUtf8(GetSourcePath()), ImportFlags);

        if (mScene == nullptr || (mScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !mScene->HasMeshes() || mScene->mRootNode == nullptr) {
            Error = mScene == nullptr ? mImporter.GetErrorString() : "The source contains no complete mesh data";

            return false;
        }

        // 원본 전역 공간에서 엔진 공간으로: 축 변환 → 임포트 이동·회전·스케일 순서로 적용합니다.
        // OBJ에는 단위 정보가 없고 glTF는 항상 미터이므로 단위 변환 옵션은 적용할 값이 없습니다.
        mBasis = Asset::FMeshImportTransform::MakeYUpConversion(Option) * Option.GetImportTransform();

        if (!ReadMaterials()) {
            return false;
        }

        // 스켈레톤은 메시보다 먼저 읽어야 ReadMesh가 본 이름을 결과 본 인덱스로 바꿀 수 있습니다.
        if (Option.mImportSkeleton && (!CollectBindPoses(*mScene->mRootNode, {}) || !ReadSkeleton(*mScene->mRootNode, -1, {}))) {
            return false;
        }

        // 메시를 병합하면 모든 메시 노드가 이어 붙을 결과 메시 하나를 원본 파일 이름으로 미리 만듭니다.
        if (Option.mCombineMeshes) {
            GetResult().GetMeshes().emplace_back(Core::FilePath::ToUtf8(GetSourcePath().stem()));
        }

        return ReadNode(*mScene->mRootNode, {});
    }

    /**
     * @brief 장면의 Assimp 재질마다 기본 색상·불투명도·양면 여부와 기본 색상·노멀 텍스처를 읽어 재질 목록에 추가합니다.
     */
    bool Asset::FAssimpMeshImporter::ReadMaterials() {
        Core::TArray<Asset::FMeshImportMaterial>& Materials{GetResult().GetMaterials()};
        Materials.reserve(mScene->mNumMaterials);

        for (Core::Uint32 Index{}; Index < mScene->mNumMaterials; ++Index) {
            const aiMaterial& Material{*mScene->mMaterials[Index]};
            aiString MaterialName{};

            Material.Get(AI_MATKEY_NAME, MaterialName);

            const Core::FString Name{MaterialName.length == 0 ? "Material_" + std::to_string(Index) : Core::FString{MaterialName.C_Str()}};
            aiColor4D Color{1.0f, 1.0f, 1.0f, 1.0f};

            aiGetMaterialColor(&Material, AI_MATKEY_COLOR_DIFFUSE, &Color);

            float Opacity{1.0f};

            Material.Get(AI_MATKEY_OPACITY, Opacity);

            Core::Int32 TwoSided{};

            Material.Get(AI_MATKEY_TWOSIDED, TwoSided);

            Asset::FMeshImportTexture SpecularTexture{ReadTexture(Material, aiTextureType_SPECULAR, aiTextureType_SPECULAR)};
            Asset::FMeshImportTexture EmissiveTexture{ReadTexture(Material, aiTextureType_EMISSIVE, aiTextureType_EMISSION_COLOR)};
            Asset::FMeshImportTexture ReflectionTexture{ReadTexture(Material, aiTextureType_REFLECTION, aiTextureType_REFLECTION)};
            const auto ReadColor{[&](const char* Key, Core::Uint32 Type, Core::Uint32 PropertyIndex, const Core::FVector4& Default) {
                aiColor4D Value{Default.x, Default.y, Default.z, Default.w};

                aiGetMaterialColor(&Material, Key, Type, PropertyIndex, &Value);

                return Core::FVector4{Value.r, Value.g, Value.b, 1.0f};
            }};

            const float SpecularDefault{SpecularTexture.GetSource() == Asset::EMeshImportTextureSource::None ? 0.0f : 1.0f};
            const float EmissiveDefault{EmissiveTexture.GetSource() == Asset::EMeshImportTextureSource::None ? 0.0f : 1.0f};
            const Core::FVector4 SpecularColor{ReadColor(AI_MATKEY_COLOR_SPECULAR, {SpecularDefault, SpecularDefault, SpecularDefault, 1.0f})};
            const Core::FVector4 AmbientColor{ReadColor(AI_MATKEY_COLOR_AMBIENT, {1.0f, 1.0f, 1.0f, 1.0f})};
            const Core::FVector4 EmissiveColor{ReadColor(AI_MATKEY_COLOR_EMISSIVE, {EmissiveDefault, EmissiveDefault, EmissiveDefault, 1.0f})};
            const Core::FVector4 ReflectionColor{ReadColor(AI_MATKEY_COLOR_REFLECTIVE, {1.0f, 1.0f, 1.0f, 1.0f})};

            float SpecularStrength{1.0f};
            float Shininess{32.0f};
            float ReflectionStrength{ReflectionTexture.GetSource() == Asset::EMeshImportTextureSource::None ? 0.0f : 1.0f};

            Material.Get(AI_MATKEY_SHININESS_STRENGTH, SpecularStrength);
            Material.Get(AI_MATKEY_SHININESS, Shininess);
            Material.Get(AI_MATKEY_REFLECTIVITY, ReflectionStrength);

            Asset::FMeshImportMaterial Imported{Name, Core::FVector4{Color.r, Color.g, Color.b, Opacity}, TwoSided != 0, ReadTexture(Material, aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE), ReadTexture(Material, aiTextureType_NORMALS, aiTextureType_HEIGHT)};

            if (!Imported.SetLighting(SpecularColor, SpecularStrength, Shininess, AmbientColor, EmissiveColor, ReflectionColor, ReflectionStrength, std::move(SpecularTexture), std::move(EmissiveTexture), std::move(ReflectionTexture))) {
                GetError() = "The source material contains invalid lighting properties: " + Name;

                return false;
            }

            Materials.push_back(std::move(Imported));
        }

        return true;
    }

}
