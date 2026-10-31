#include "Asset/Import/FFBXLoader.h"
#include "Core/FilePath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <d3d11.h>
#include <limits>
#include <utility>

using namespace fbxsdk;

namespace Asset {

    Asset::FFBXLoader::FFBXLoader(std::filesystem::path Source)
		: IMeshImporter{std::move(Source)} {
    }

    Asset::FFBXLoader::~FFBXLoader() {
        if (mManager != nullptr) {
            mManager->Destroy();
        }
    }

    Core::FVector Asset::FFBXLoader::TransformDirection(const FbxAMatrix& Matrix, const FbxVector4& Vector) const {
        const FbxVector4 Transformed{Matrix.MultT(Vector) - Matrix.GetT()};
        return {static_cast<float>(Transformed[0]), static_cast<float>(Transformed[1]), static_cast<float>(Transformed[2])};
    }

    bool Asset::FFBXLoader::ReadMaterial(FbxSurfaceMaterial* Material) {
        Asset::FMeshImportResult& Result{GetResult()};
        Core::FString& Error{GetError()};

        Core::FString Name{Material == nullptr ? "Default" : Material->GetName()};

        if (Name.empty()) {
            Name = "Material_" + std::to_string(Result.GetMaterials().size());
        }

        Core::FVector4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};
        bool TwoSided{};
        Asset::FMeshImportTexture BaseColorTexture{};
        Asset::FMeshImportTexture NormalTexture{};

        if (auto* Lambert{FbxCast<FbxSurfaceLambert>(Material)}; Lambert != nullptr) {
            const FbxDouble3 Color{Lambert->Diffuse.Get()};
            const double Factor{Lambert->DiffuseFactor.Get()};
            const FbxDouble3 Transparent{Lambert->TransparentColor.Get()};
            const double Opacity{1.0 - Lambert->TransparencyFactor.Get() * (Transparent[0] + Transparent[1] + Transparent[2]) / 3.0};
            BaseColor = {static_cast<float>(Color[0] * Factor), static_cast<float>(Color[1] * Factor), static_cast<float>(Color[2] * Factor), static_cast<float>(Opacity)};
        }

        if (Material != nullptr) {
            const FbxProperty DoubleSided{Material->FindProperty("DoubleSided")};
            TwoSided = DoubleSided.IsValid() && DoubleSided.Get<FbxBool>();
            // 재질 슬롯별로 받아들이는 텍스처 형태입니다. 렌더러가 새 형태를 지원하면 여기에 추가합니다.
            constexpr std::array BaseColorShapes{Asset::EMeshImportTextureShape::Texture2D};
            constexpr std::array NormalShapes{Asset::EMeshImportTextureShape::Texture2D};
            const auto Read{[&](const char* Property, std::span<const Asset::EMeshImportTextureShape> Accepted, Asset::FMeshImportTexture& Texture) {
                return mImageProcessor.ReadTexture(Material->FindProperty(Property), Accepted, Texture, Error);
            }};

            if (!Read(FbxSurfaceMaterial::sDiffuse, BaseColorShapes, BaseColorTexture) || !Read(FbxSurfaceMaterial::sNormalMap, NormalShapes, NormalTexture)) {
                return false;
            }

            if (NormalTexture.GetSource() == Asset::EMeshImportTextureSource::None && !Read(FbxSurfaceMaterial::sBump, NormalShapes, NormalTexture)) {
                return false;
            }
        }

        if (!BaseColor.IsFinite()) {
            Error = "The FBX material contains an invalid color: " + Name;
            return false;
        }

        Result.GetMeshData().GetMaterialSlots().emplace_back(Name, Core::FString{});
        Result.GetMaterials().emplace_back(std::move(Name), BaseColor, TwoSided, std::move(BaseColorTexture), std::move(NormalTexture));
        return true;
    }

    bool Asset::FFBXLoader::FindMaterialIndex(FbxSurfaceMaterial* Material, Core::Uint32& Index) {
        auto Existing{mMaterialIndices.find(Material)};

        if (Existing == mMaterialIndices.end()) {
            const auto MaterialIndex{static_cast<Core::Uint32>(GetResult().GetMaterials().size())};

            if (!ReadMaterial(Material)) {
                return false;
            }

            Existing = mMaterialIndices.emplace(Material, MaterialIndex).first;
        }

        Index = Existing->second;
        return true;
    }

    bool Asset::FFBXLoader::ReadMesh(FbxNode& Node, FbxMesh& Mesh) {
        Asset::FMeshImportResult& Result{GetResult()};
        Core::FString& Error{GetError()};

        Asset::FMeshData& Data{Result.GetMeshData()};
        const std::size_t AddedVertices{static_cast<std::size_t>(Mesh.GetPolygonCount()) * 3};
        constexpr std::size_t MaximumVertices{std::numeric_limits<Core::Uint32>::max() / (sizeof(float) * 7)};
        const std::size_t VertexCount{Data.GetPositions().size()};
        const std::size_t UVCount{std::max(Data.GetUVChannels().size(), static_cast<std::size_t>(Mesh.GetElementUVCount()))};

        if (!Mesh.IsTriangleMesh() || Mesh.GetControlPointsCount() == 0 || AddedVertices > MaximumVertices - VertexCount || UVCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount + AddedVertices > std::numeric_limits<Core::Uint32>::max() / (std::max<std::size_t>(1, UVCount) * sizeof(float) * 2)) {
            Error = "The FBX mesh is invalid or exceeds mesh buffer limits";
            return false;
        }

        if (AddedVertices == 0) {
            return true;
        }

        FbxAMatrix Geometry{};
        Geometry.SetT(Node.GetGeometricTranslation(FbxNode::eSourcePivot));
        Geometry.SetR(Node.GetGeometricRotation(FbxNode::eSourcePivot));
        Geometry.SetS(Node.GetGeometricScaling(FbxNode::eSourcePivot));
        const FbxAMatrix Transform{Node.EvaluateGlobalTransform(FBXSDK_TIME_ZERO) * Geometry};
        const double Determinant{FbxMatrix{Transform}.Determinant()};

        if (!std::isfinite(Determinant) || std::abs(Determinant) < 1.0e-12) {
            Error = "The FBX mesh has a non-invertible node transform";
            return false;
        }

        const FbxAMatrix NormalTransform{Transform.Inverse().Transpose()};
        const bool Mirrored{Determinant < 0.0};

        if (Mesh.GetElementNormalCount() == 0 && !Mesh.GenerateNormals()) {
            Error = "Cannot generate FBX mesh normals";
            return false;
        }

        if (Mesh.GetElementUVCount() > 0 && Mesh.GetElementTangentCount() == 0) {
            Mesh.GenerateTangentsData(0, false);
        }

        Core::TArray<Core::Uint32> NodeMaterials{};
        const auto AddMaterial{[&](FbxSurfaceMaterial* Material) {
            Core::Uint32 MaterialIndex{};

            if (!FindMaterialIndex(Material, MaterialIndex)) {
                return false;
            }

            NodeMaterials.push_back(MaterialIndex);
            return true;
        }};

        for (int Index{}; Index < std::max(1, Node.GetMaterialCount()); ++Index) {
            if (!AddMaterial(Node.GetMaterialCount() == 0 ? nullptr : Node.GetMaterial(Index))) {
                return false;
            }
        }

        Data.GetPositions().reserve(VertexCount + AddedVertices);
        Data.GetNormals().reserve(VertexCount + AddedVertices);
        Data.GetTangents().reserve(VertexCount + AddedVertices);
        Data.GetColors().reserve(VertexCount + AddedVertices);
        Data.GetUVChannels().resize(UVCount);

        for (auto& Channel : Data.GetUVChannels()) {
            Channel.resize(VertexCount);
            Channel.reserve(VertexCount + AddedVertices);
        }

        Core::TArray<Core::TArray<Core::Uint32>> Sections{};
        Sections.resize(NodeMaterials.size());

        for (int Polygon{}; Polygon < Mesh.GetPolygonCount(); ++Polygon) {
            int LocalMaterial{};

            if (const auto* Element{Mesh.GetElementMaterial()}; Element != nullptr && Node.GetMaterialCount() > 0) {
                const int MaterialEntry{Element->GetMappingMode() == FbxLayerElement::eAllSame ? 0 : Polygon};

                if ((Element->GetMappingMode() != FbxLayerElement::eAllSame && Element->GetMappingMode() != FbxLayerElement::eByPolygon) || MaterialEntry >= Element->GetIndexArray().GetCount()) {
                    Error = "The FBX mesh contains an invalid material mapping";
                    return false;
                }

                LocalMaterial = Element->GetIndexArray().GetAt(MaterialEntry);
            }

            if (LocalMaterial < -1 || LocalMaterial >= std::max(1, Node.GetMaterialCount())) {
                Error = "The FBX mesh references an invalid material slot";
                return false;
            }

            if (LocalMaterial == -1) {
                LocalMaterial = Node.GetMaterialCount();

                if (NodeMaterials.size() == static_cast<std::size_t>(LocalMaterial)) {
                    if (!AddMaterial(nullptr)) {
                        return false;
                    }

                    Sections.emplace_back();
                }
            }

            for (int Corner{}; Corner < 3; ++Corner) {
                const int SourceCorner{Mirrored ? 2 - Corner : Corner};
                const int ControlPoint{Mesh.GetPolygonVertex(Polygon, SourceCorner)};
                const int PolygonVertex{Mesh.GetPolygonVertexIndex(Polygon) + SourceCorner};
                const auto Read{[&](const auto* Element, auto& Value) {
                    return ReadElement(Element, ControlPoint, Polygon, PolygonVertex, Value);
                }};

                if (ControlPoint < 0 || ControlPoint >= Mesh.GetControlPointsCount()) {
                    Error = "The FBX mesh contains an invalid control point";
                    return false;
                }

                const FbxVector4 Position{Transform.MultT(Mesh.GetControlPointAt(ControlPoint))};
                const Core::FVector Vertex{static_cast<float>(Position[0]), static_cast<float>(Position[1]), static_cast<float>(Position[2])};
                FbxVector4 SourceNormal{};
                FbxVector4 SourceTangent{};
                FbxVector4 SourceBitangent{};
                FbxColor Color{1.0, 1.0, 1.0, 1.0};

                if (!Read(Mesh.GetElementNormal(), SourceNormal) || !Read(Mesh.GetElementTangent(), SourceTangent) || !Read(Mesh.GetElementBinormal(), SourceBitangent) || !Read(Mesh.GetElementVertexColor(), Color)) {
                    Error = "The FBX mesh contains an invalid vertex attribute mapping";
                    return false;
                }

                Core::FVector Normal{TransformDirection(NormalTransform, SourceNormal)};

                if (!Normal.IsFinite() || !Normal.Normalize()) {
                    Error = "The FBX mesh contains an invalid normal";
                    return false;
                }

                const Core::FVector TangentDirection{TransformDirection(Transform, SourceTangent)};
                const Core::FVector Bitangent{TransformDirection(Transform, SourceBitangent)};
                Core::FVector Tangent{TangentDirection - Normal * Normal.Dot(TangentDirection)};
                float Handedness{1.0f};

                if (Mesh.GetElementTangent() != nullptr && (!Tangent.IsFinite() || !Bitangent.IsFinite())) {
                    Error = "The FBX mesh contains an invalid tangent";
                    return false;
                }

                if (!Tangent.Normalize()) {
                    const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
                    Tangent = Axis.Cross(Normal).Normalized();
                } else if (Mesh.GetElementBinormal() != nullptr) {
                    Handedness = Normal.Cross(Tangent).Dot(Bitangent) > 0.0f ? -1.0f : 1.0f;
                } else {
                    Handedness = static_cast<float>(SourceTangent[3]) * (Mirrored ? 1.0f : -1.0f);
                    Handedness = Handedness < 0.0f ? -1.0f : 1.0f;
                }

                const Core::FVector4 VertexColor{static_cast<float>(Color.mRed), static_cast<float>(Color.mGreen), static_cast<float>(Color.mBlue), static_cast<float>(Color.mAlpha)};

                if (!Vertex.IsFinite() || !VertexColor.IsFinite()) {
                    Error = "The FBX mesh contains invalid vertex attributes";
                    return false;
                }

                Sections[LocalMaterial].push_back(static_cast<Core::Uint32>(Data.GetPositions().size()));
                Data.GetPositions().push_back(Vertex);
                Data.GetNormals().push_back(Normal);
                Data.GetTangents().push_back({Tangent, Handedness});
                Data.GetColors().push_back(VertexColor);

                for (std::size_t Channel{}; Channel < UVCount; ++Channel) {
                    const bool HasChannel{Channel < static_cast<std::size_t>(Mesh.GetElementUVCount())};
                    FbxVector2 UV{};

                    if (HasChannel && !Read(Mesh.GetElementUV(static_cast<int>(Channel)), UV)) {
                        Error = "The FBX mesh contains an invalid UV mapping";
                        return false;
                    }

                    const Core::FVector2 Coordinates{static_cast<float>(UV[0]), HasChannel ? 1.0f - static_cast<float>(UV[1]) : 0.0f};

                    if (!Coordinates.IsFinite()) {
                        Error = "The FBX mesh contains invalid texture coordinates";
                        return false;
                    }

                    Data.GetUVChannels()[Channel].push_back(Coordinates);
                }
            }
        }

        for (std::size_t Index{}; Index < Sections.size(); ++Index) {
            const auto& Section{Sections[Index]};

            if (!Section.empty()) {
                Data.GetSubMeshes().emplace_back(static_cast<Core::Uint32>(Data.GetIndices().size()), static_cast<Core::Uint32>(Section.size()), NodeMaterials[Index]);
                Data.GetIndices().insert(Data.GetIndices().end(), Section.begin(), Section.end());
            }
        }

        return true;
    }

    bool Asset::FFBXLoader::ReadNode(FbxNode& Node) {
        if (auto* Mesh{Node.GetMesh()}; Mesh != nullptr && !ReadMesh(Node, *Mesh)) {
            return false;
        }

        for (int Index{}; Index < Node.GetChildCount(); ++Index) {
            if (!ReadNode(*Node.GetChild(Index))) {
                return false;
            }
        }

        return true;
    }

    bool Asset::FFBXLoader::ImportSource() {
        Core::FString& Error{GetError()};
        mManager = FbxManager::Create();

        if (mManager == nullptr) {
            Error = "Cannot create the FBX SDK manager";
            return false;
        }

		// FBX Load 의 IOSettings을 생성하고 애니메이션을 비활성화합니다.
        auto* Settings{FbxIOSettings::Create(mManager, IOSROOT)};
        mManager->SetIOSettings(Settings);
        Settings->SetBoolProp(IMP_FBX_ANIMATION, false);

		// FBX Load 의 Importer와 Scene을 생성합니다.
        mImporter = FbxImporter::Create(mManager, "");
        mScene = FbxScene::Create(mManager, "");

		// FBX Load 의 Importer를 초기화하고 제대로 열렸는지, FBX 파일인지 확인합니다.
        if (!mImporter->Initialize(Core::FilePath::ToUtf8(GetSourcePath()).c_str(), -1, Settings) || !mImporter->IsFBX()) {
            Error = "Cannot open the FBX source: " + Core::FString{mImporter->GetStatus().GetErrorString()};
            return false;
        }

		// Import 중 내장 텍스처 바이트를 이미지 처리기가 보관하도록 콜백을 등록합니다.
        mImageProcessor.RegisterEmbeddedCallback(*mManager, *mImporter);

		// FBX Importer 설정이 완료되었으로, FBX Scene을 읽어옵니다.
        if (!mImporter->Import(mScene)) {
            Error = "Cannot read the FBX scene: " + Core::FString{mImporter->GetStatus().GetErrorString()};
            return false;
        }

        // Import가 끝나 장면 연결과 내장 바이트가 모두 준비되었으므로 파일 텍스처를 판별합니다.
        mImageProcessor.ResolveTextures(*mScene, GetSourceDirectory());

        const FbxAxisSystem AxisSystem{FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded};
        AxisSystem.DeepConvertScene(mScene);
        FbxSystemUnit::m.ConvertScene(mScene);
        FbxGeometryConverter Converter{mManager};

        if (!Converter.Triangulate(mScene, true)) {
            Error = "Cannot triangulate the FBX scene";
            return false;
        }

        if (mScene->GetRootNode() == nullptr) {
            Error = "The FBX scene has no root node";
            return false;
        }

        return ReadNode(*mScene->GetRootNode());
    }

}
