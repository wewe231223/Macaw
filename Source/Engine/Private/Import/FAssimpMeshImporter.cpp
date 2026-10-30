/**
 * @file FAssimpMeshImporter.cpp
 * @brief Assimp로 원본 메시를 삼각형·좌수 좌표계로 변환하고 재질과 텍스처 참조를 수집합니다.
 * @ingroup Engine
 */

#include "Engine/Import/FAssimpMeshImporter.h"
#include "Engine/Import/FMeshImportResult.h"
#include "Core/FilePath.h"
#include "FMeshImportIOSystem.h"

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

namespace Engine {

    /**
     * @brief 이 구현 파일에서만 사용하는 검증·변환·설정 보조 함수를 제공합니다.
     */
    namespace {
        /**
         * @brief 재질의 우선·대체 텍스처 슬롯에서 외부 경로 또는 원본 장면의 내장 참조를 수집합니다.
         * @param[in] Scene Assimp가 로드한 유효한 원본 장면입니다.
         * @param[in] Material 텍스처 슬롯을 조회할 원본 Assimp 재질입니다.
         * @param[in] PrimaryType 우선 조회할 Assimp 재질 텍스처 슬롯 종류입니다.
         * @param[in] FallbackType 우선 슬롯이 없을 때 조회할 대체 텍스처 슬롯 종류입니다.
         * @param[in] Directory 대상 에셋 또는 원본 텍스처의 기준 디렉터리입니다.
         * @return 참조가 없으면 None이며 나머지는 출처를 구분한 텍스처 참조 값입니다.
         */
        Engine::FMeshImportTexture ReadTexture(const aiScene& Scene, const aiMaterial& Material, aiTextureType PrimaryType, aiTextureType FallbackType, const std::filesystem::path& Directory) {
            aiString Texture{};

            if (Material.GetTexture(PrimaryType, 0, &Texture) != AI_SUCCESS && Material.GetTexture(FallbackType, 0, &Texture) != AI_SUCCESS) {
                return {};
            }

            Core::FString Reference{Texture.C_Str()};
            std::replace(Reference.begin(), Reference.end(), '\\', '/');

            if (Reference.starts_with("*") || Scene.GetEmbeddedTexture(Texture.C_Str()) != nullptr) {
                return Engine::FMeshImportTexture::FromEmbedded(std::move(Reference));
            }

            return Engine::FMeshImportTexture{(Directory / Core::FilePath::FromUtf8(Reference)).lexically_normal()};
        }

        /**
         * @brief Assimp 장면의 메시들을 하나의 검증된 정점·인덱스·서브메시 데이터로 병합합니다.
         * @param[in] Scene Assimp가 로드한 유효한 원본 장면입니다.
         * @param[in,out] MeshData 병합한 정점·삼각형·서브메시와 경계를 기록할 데이터입니다.
         * @param[out] Error 실패 원인을 기록할 문자열입니다.
         * @return 속성·삼각형·범위 검증을 통과하면 true이며 오류이면 false입니다.
         */
        bool ReadMesh(const aiScene* Scene, Engine::FMeshData& MeshData, Core::FString& Error) {
            constexpr std::size_t MaximumBytes{std::numeric_limits<Core::Uint32>::max()};
            const std::size_t MaximumVertices{MaximumBytes / sizeof(std::array<float, 7>)};
            constexpr std::size_t MaximumIndices{MaximumBytes / sizeof(Core::Uint32)};
            std::size_t VertexCount{};
            std::size_t IndexCount{};
            Core::Uint32 UVChannelCount{};

            for (Core::Uint32 MeshIndex{}; MeshIndex < Scene->mNumMeshes; ++MeshIndex) {
                const aiMesh& SourceMesh{*Scene->mMeshes[MeshIndex]};

                if (!SourceMesh.HasPositions() || !SourceMesh.HasFaces() || SourceMesh.mNumVertices > MaximumVertices - VertexCount || SourceMesh.mNumFaces > (MaximumIndices - IndexCount) / 3 || SourceMesh.mMaterialIndex >= Scene->mNumMaterials) {
                    Error = "The source contains an empty mesh or exceeds mesh buffer limits";
                    return false;
                }

                VertexCount += SourceMesh.mNumVertices;
                IndexCount += static_cast<std::size_t>(SourceMesh.mNumFaces) * 3;

                for (Core::Uint32 Channel{}; Channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++Channel) {
                    if (SourceMesh.HasTextureCoords(Channel)) {
                        UVChannelCount = std::max(UVChannelCount, Channel + 1);
                    }
                }
            }

            const std::size_t UVStride{std::max<std::size_t>(1, UVChannelCount) * sizeof(float) * 2};

            if (VertexCount == 0 || IndexCount == 0 || UVStride > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES || VertexCount > MaximumBytes / UVStride) {
                Error = "The source contains no triangles or exceeds mesh buffer limits";
                return false;
            }

            Core::TArray<Core::FVector>& Positions{MeshData.GetPositions()};
            Core::TArray<Core::FVector>& Normals{MeshData.GetNormals()};
            Core::TArray<Core::FVector4>& Tangents{MeshData.GetTangents()};
            Core::TArray<Core::TArray<Core::FVector2>>& UVChannels{MeshData.GetUVChannels()};
            Core::TArray<Core::FVector4>& Colors{MeshData.GetColors()};
            Core::TArray<Core::Uint32>& Indices{MeshData.GetIndices()};
            Positions.reserve(VertexCount);
            Normals.reserve(VertexCount);
            Tangents.reserve(VertexCount);
            Colors.reserve(VertexCount);
            Indices.reserve(IndexCount);
            UVChannels.resize(UVChannelCount);

            for (Core::TArray<Core::FVector2>& Channel : UVChannels) {
                Channel.reserve(VertexCount);
            }

            for (Core::Uint32 MaterialIndex{}; MaterialIndex < Scene->mNumMaterials; ++MaterialIndex) {
                aiString Name{};
                Scene->mMaterials[MaterialIndex]->Get(AI_MATKEY_NAME, Name);

                const Core::FString MaterialName{Name.length == 0 ? "Material_" + std::to_string(MaterialIndex) : Core::FString{Name.C_Str()}};
                MeshData.GetMaterialSlots().emplace_back(MaterialName, Core::FString{});
            }

            for (Core::Uint32 MeshIndex{}; MeshIndex < Scene->mNumMeshes; ++MeshIndex) {
                const aiMesh& SourceMesh{*Scene->mMeshes[MeshIndex]};
                const Core::Uint32 BaseVertex{static_cast<Core::Uint32>(Positions.size())};
                const Core::Uint32 FirstIndex{static_cast<Core::Uint32>(Indices.size())};

                for (Core::Uint32 VertexIndex{}; VertexIndex < SourceMesh.mNumVertices; ++VertexIndex) {
                    const aiVector3D& Position{SourceMesh.mVertices[VertexIndex]};
                    Positions.push_back({Position.x, Position.y, Position.z});

                    const Core::FVector Normal{SourceMesh.HasNormals() ? Core::FVector{SourceMesh.mNormals[VertexIndex].x, SourceMesh.mNormals[VertexIndex].y, SourceMesh.mNormals[VertexIndex].z}.Normalized() : Core::FVector{0.0f, 0.0f, 1.0f}};
                    Normals.push_back(Normal);

                    const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
                    Core::FVector Tangent{Axis.Cross(Normal).Normalized()};
                    float Handedness{1.0f};

                    if (SourceMesh.HasTangentsAndBitangents()) {
                        const Core::FVector SourceTangent{SourceMesh.mTangents[VertexIndex].x, SourceMesh.mTangents[VertexIndex].y, SourceMesh.mTangents[VertexIndex].z};
                        const Core::FVector Bitangent{SourceMesh.mBitangents[VertexIndex].x, SourceMesh.mBitangents[VertexIndex].y, SourceMesh.mBitangents[VertexIndex].z};
                        Core::FVector OrthogonalTangent{SourceTangent - Normal * Normal.Dot(SourceTangent)};

                        if (OrthogonalTangent.Normalize()) {
                            Tangent = OrthogonalTangent;
                            Handedness = Normal.Cross(Tangent).Dot(Bitangent) < 0.0f ? -1.0f : 1.0f;
                        }
                    }

                    Tangents.push_back({Tangent, Handedness});

                    if (SourceMesh.HasVertexColors(0)) {
                        const aiColor4D& Color{SourceMesh.mColors[0][VertexIndex]};
                        Colors.push_back({Color.r, Color.g, Color.b, Color.a});
                    } else {
                        Colors.push_back({1.0f, 1.0f, 1.0f, 1.0f});
                    }

                    for (Core::Uint32 Channel{}; Channel < UVChannelCount; ++Channel) {
                        const Core::FVector2 UV{SourceMesh.HasTextureCoords(Channel) ? Core::FVector2{SourceMesh.mTextureCoords[Channel][VertexIndex].x, SourceMesh.mTextureCoords[Channel][VertexIndex].y} : Core::FVector2{}};
                        UVChannels[Channel].push_back(UV);
                    }
                }

                for (Core::Uint32 FaceIndex{}; FaceIndex < SourceMesh.mNumFaces; ++FaceIndex) {
                    const aiFace& Face{SourceMesh.mFaces[FaceIndex]};

                    if (Face.mNumIndices != 3) {
                        Error = "The source contains a face that could not be triangulated";
                        return false;
                    }

                    for (Core::Uint32 Corner{}; Corner < 3; ++Corner) {
                        if (Face.mIndices[Corner] >= SourceMesh.mNumVertices) {
                            Error = "The source contains an invalid vertex index";
                            return false;
                        }

                        Indices.push_back(BaseVertex + Face.mIndices[Corner]);
                    }
                }

                MeshData.GetSubMeshes().emplace_back(FirstIndex, static_cast<Core::Uint32>(SourceMesh.mNumFaces * 3), SourceMesh.mMaterialIndex);
            }

            Core::FVector Minimum{Positions.front()};
            Core::FVector Maximum{Minimum};

            for (std::size_t Index{}; Index < Positions.size(); ++Index) {
                if (!Positions[Index].IsFinite() || !Normals[Index].IsFinite() || !Tangents[Index].IsFinite() || !Colors[Index].IsFinite()) {
                    Error = "The source contains invalid vertex attributes";
                    return false;
                }

                Minimum = Minimum.Min(Positions[Index]);
                Maximum = Maximum.Max(Positions[Index]);

                for (const auto& Channel : UVChannels) {
                    if (!Channel[Index].IsFinite()) {
                        Error = "The source contains invalid texture coordinates";
                        return false;
                    }
                }
            }

            MeshData.SetBounds({Minimum, Maximum});

            return true;
        }
    }

    bool Engine::FAssimpMeshImporter::Import(const std::filesystem::path& Source, Engine::FMeshImportResult& Result, Core::FString& Error) const {
        Error.clear();

        if (Source.empty()) {
            Error = "A mesh source path is required";
            return false;
        }

        std::error_code Code{};
        const auto SourcePath{std::filesystem::absolute(Source, Code).lexically_normal()};

        if (Code) {
            Error = "Cannot resolve the mesh source path: " + Code.message();
            return false;
        }

        Assimp::Importer Importer{};
        Importer.SetIOHandler(new Engine::FMeshImportIOSystem{});
        Importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        Importer.SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);
        Importer.SetPropertyBool(AI_CONFIG_PP_FID_IGNORE_TEXTURECOORDS, true);

        constexpr Core::Uint32 ImportFlags{aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_SortByPType | aiProcess_PreTransformVertices | aiProcess_ConvertToLeftHanded | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure};
        const aiScene* Scene{Importer.ReadFile(Core::FilePath::ToUtf8(SourcePath), ImportFlags)};

        if (Scene == nullptr || (Scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !Scene->HasMeshes()) {
            Error = Scene == nullptr ? Importer.GetErrorString() : "The source contains no complete mesh data";
            return false;
        }

        Engine::FMeshImportResult Imported{};

        if (!ReadMesh(Scene, Imported.GetMeshData(), Error)) {
            return false;
        }

        Core::TArray<Engine::FMeshImportMaterial>& Materials{Imported.GetMaterials()};
        Materials.reserve(Scene->mNumMaterials);

        for (Core::Uint32 Index{}; Index < Scene->mNumMaterials; ++Index) {
            const aiMaterial& Material{*Scene->mMaterials[Index]};
            const Core::FString& Name{Imported.GetMeshData().GetMaterialSlots()[Index].GetName()};
            aiColor4D Color{1.0f, 1.0f, 1.0f, 1.0f};
            aiGetMaterialColor(&Material, AI_MATKEY_COLOR_DIFFUSE, &Color);

            float Opacity{1.0f};
            Material.Get(AI_MATKEY_OPACITY, Opacity);

            Core::Int32 TwoSided{};
            Material.Get(AI_MATKEY_TWOSIDED, TwoSided);
            Materials.emplace_back(Name, Core::FVector4{Color.r, Color.g, Color.b, Opacity}, TwoSided != 0, ReadTexture(*Scene, Material, aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, SourcePath.parent_path()), ReadTexture(*Scene, Material, aiTextureType_NORMALS, aiTextureType_HEIGHT, SourcePath.parent_path()));
        }

        Result = std::move(Imported);

        return true;
    }

}
