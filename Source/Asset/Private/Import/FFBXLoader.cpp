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
        // FbxManager::Destroy는 이 Manager로 생성한 Importer, Scene, IOSettings까지 함께 해제하므로 Manager만 해제합니다.
        if (mManager != nullptr) {
            mManager->Destroy();
        }
    }

    Core::FVector Asset::FFBXLoader::TransformDirection(const FbxAMatrix& Matrix, const FbxVector4& Vector) const {
        // MultT는 이동까지 포함해 점처럼 변환하므로, 결과에서 이동 성분(GetT)을 빼서 회전·스케일만 적용된 방향을 얻습니다.
        const FbxVector4 Transformed{Matrix.MultT(Vector) - Matrix.GetT()};
        // FBX 중간 좌표계(오른손)에서 엔진 좌표계(왼손)로 옮기기 위해 Y를 반전합니다. ImportSource의 축 변환 설명을 참고합니다.
        return {static_cast<float>(Transformed[0]), -static_cast<float>(Transformed[1]), static_cast<float>(Transformed[2])};
    }

    bool Asset::FFBXLoader::ReadMaterial(FbxSurfaceMaterial* Material) {
        Asset::FMeshImportResult& Result{GetResult()};
        Core::FString& Error{GetError()};

        // 재질이 없는 메시(nullptr)는 "Default" 재질로 취급합니다.
        Core::FString Name{Material == nullptr ? "Default" : Material->GetName()};

        // FBX 재질에 이름이 없으면 현재까지 읽은 재질 수를 붙여 고유한 이름을 만듭니다.
        if (Name.empty()) {
            Name = "Material_" + std::to_string(Result.GetMaterials().size());
        }

        // FBX에서 값을 읽지 못한 경우의 기본값입니다. 기본 색상은 흰색·불투명, 단면, 텍스처 없음이고,
        // 조명 속성은 스페큘러·반사 강도 0, 발광 검정, Shininess 32입니다.
        Core::FVector4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};
        bool TwoSided{};
        Asset::FMeshImportTexture BaseColorTexture{};
        Asset::FMeshImportTexture NormalTexture{};
        Core::FVector4 SpecularColor{1.0f, 1.0f, 1.0f, 1.0f};
        Core::FVector4 AmbientColor{1.0f, 1.0f, 1.0f, 1.0f};
        Core::FVector4 EmissiveColor{0.0f, 0.0f, 0.0f, 1.0f};
        Core::FVector4 ReflectionColor{1.0f, 1.0f, 1.0f, 1.0f};
        float SpecularStrength{};
        float Shininess{32.0f};
        float ReflectionStrength{};
        Asset::FMeshImportTexture SpecularTexture{};
        Asset::FMeshImportTexture EmissiveTexture{};
        Asset::FMeshImportTexture ReflectionTexture{};
        // FBX 색상(RGB)에 계수를 곱해 알파 1인 엔진 색상으로 바꾸는 도우미입니다.
        const auto ReadColor{[](const FbxDouble3& Color, double Factor) {
            return Core::FVector4{static_cast<float>(Color[0] * Factor), static_cast<float>(Color[1] * Factor), static_cast<float>(Color[2] * Factor), 1.0f};
        }};

        // Lambert 계열(Phong은 Lambert를 상속하므로 함께 해당) 재질이면 Diffuse·투명도·Ambient·Emissive를 읽습니다.
        // nullptr이나 다른 종류의 재질이면 FbxCast가 nullptr을 반환하여 기본값을 유지합니다.
        if (auto* Lambert{FbxCast<FbxSurfaceLambert>(Material)}; Lambert != nullptr) {
            const FbxDouble3 Color{Lambert->Diffuse.Get()};
            const double Factor{Lambert->DiffuseFactor.Get()};
            const FbxDouble3 Transparent{Lambert->TransparentColor.Get()};
            // 불투명도 = 1 - (투명 계수 × 투명 색상 RGB 평균)입니다. 투명 색상이 검정이거나 계수가 0이면 완전 불투명입니다.
            const double Opacity{1.0 - Lambert->TransparencyFactor.Get() * (Transparent[0] + Transparent[1] + Transparent[2]) / 3.0};
            // RGB는 Diffuse 색상에 DiffuseFactor를 곱한 값, A는 위에서 계산한 불투명도입니다.
            BaseColor = {static_cast<float>(Color[0] * Factor), static_cast<float>(Color[1] * Factor), static_cast<float>(Color[2] * Factor), static_cast<float>(Opacity)};
            AmbientColor = ReadColor(Lambert->Ambient.Get(), Lambert->AmbientFactor.Get());
            EmissiveColor = ReadColor(Lambert->Emissive.Get(), Lambert->EmissiveFactor.Get());
        }

        // Phong 재질이면 Lambert에 없는 스페큘러(색상·강도·Shininess)와 반사(색상·강도) 속성을 추가로 읽습니다.
        if (auto* Phong{FbxCast<FbxSurfacePhong>(Material)}; Phong != nullptr) {
            SpecularColor = ReadColor(Phong->Specular.Get(), 1.0);
            SpecularStrength = static_cast<float>(Phong->SpecularFactor.Get());
            Shininess = static_cast<float>(Phong->Shininess.Get());
            ReflectionColor = ReadColor(Phong->Reflection.Get(), 1.0);
            ReflectionStrength = static_cast<float>(Phong->ReflectionFactor.Get());
        }

        if (Material != nullptr) {
            // 양면 여부는 표준 속성이 아니라 "DoubleSided" 사용자 속성으로 저장되므로, 있으면 그 값을 사용합니다.
            const FbxProperty DoubleSided{Material->FindProperty("DoubleSided")};
            TwoSided = DoubleSided.IsValid() && DoubleSided.Get<FbxBool>();
            // 재질 슬롯별로 받아들이는 텍스처 형태입니다. 렌더러가 새 형태를 지원하면 여기에 추가합니다.
            constexpr std::array BaseColorShapes{Asset::EMeshImportTextureShape::Texture2D};
            constexpr std::array NormalShapes{Asset::EMeshImportTextureShape::Texture2D};
            // 재질의 지정한 속성에 연결된 텍스처를 이미지 처리기로 읽는 도우미입니다.
            // 텍스처가 연결되지 않은 경우에도 true를 반환하며, Texture의 Source가 None으로 남습니다.
            const auto Read{[&](const char* Property, std::span<const Asset::EMeshImportTextureShape> Accepted, Asset::FMeshImportTexture& Texture) {
                return mImageProcessor.ReadTexture(Material->FindProperty(Property), Accepted, Texture, Error);
            }};

            // Diffuse 속성은 기본 색상 텍스처로, NormalMap 속성은 노멀 텍스처로 읽습니다.
            if (!Read(FbxSurfaceMaterial::sDiffuse, BaseColorShapes, BaseColorTexture) || !Read(FbxSurfaceMaterial::sNormalMap, NormalShapes, NormalTexture)) {
                return false;
            }

            // NormalMap에 텍스처가 없으면, 일부 DCC 툴이 노멀 맵을 Bump 속성에 연결하므로 Bump 속성에서 다시 찾습니다.
            if (NormalTexture.GetSource() == Asset::EMeshImportTextureSource::None && !Read(FbxSurfaceMaterial::sBump, NormalShapes, NormalTexture)) {
                return false;
            }

            // 스페큘러·발광·반사 텍스처는 색상 텍스처이므로 기본 색상과 같은 형태 제한을 사용합니다.
            if (!Read(FbxSurfaceMaterial::sSpecular, BaseColorShapes, SpecularTexture) || !Read(FbxSurfaceMaterial::sEmissive, BaseColorShapes, EmissiveTexture) || !Read(FbxSurfaceMaterial::sReflection, BaseColorShapes, ReflectionTexture)) {
                return false;
            }
        }

        // NaN이나 무한대가 섞인 색상은 렌더링 결과를 망가뜨리므로 임포트를 실패시킵니다.
        if (!BaseColor.IsFinite()) {
            Error = "The FBX material contains an invalid color: " + Name;
            return false;
        }

        Asset::FMeshImportMaterial Imported{Name, BaseColor, TwoSided, std::move(BaseColorTexture), std::move(NormalTexture)};

        // SetLighting은 색상이 유한하고 음수가 아닌지, 강도가 0~1 범위인지 등을 검증한 뒤 조명 속성을 설정합니다.
        if (!Imported.SetLighting(SpecularColor, SpecularStrength, Shininess, AmbientColor, EmissiveColor, ReflectionColor, ReflectionStrength, std::move(SpecularTexture), std::move(EmissiveTexture), std::move(ReflectionTexture))) {
            Error = "The FBX material contains invalid lighting properties: " + Name;
            return false;
        }

        // 메시 데이터의 재질 슬롯과 임포트 결과의 재질 목록에 같은 순서로 추가하여, 두 목록의 인덱스가 항상 일치하도록 합니다.
        Result.GetMeshData().GetMaterialSlots().emplace_back(Name, Core::FString{});
        Result.GetMaterials().push_back(std::move(Imported));
        return true;
    }

    bool Asset::FFBXLoader::FindMaterialIndex(FbxSurfaceMaterial* Material, Core::Uint32& Index) {
        // 여러 노드가 같은 FBX 재질을 공유할 수 있으므로, 이미 읽은 재질은 캐시된 인덱스를 재사용합니다.
        auto Existing{mMaterialIndices.find(Material)};

        if (Existing == mMaterialIndices.end()) {
            // ReadMaterial이 재질 목록 끝에 추가하므로, 추가 전의 목록 크기가 새 재질의 인덱스가 됩니다.
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

        // 모든 FBX 메시는 하나의 결과 메시 데이터에 이어 붙입니다.
        Asset::FMeshData& Data{Result.GetMeshData()};
        // 정점을 공유하지 않고 삼각형의 각 모서리(Corner)마다 정점을 하나씩 만들므로, 추가될 정점 수는 삼각형 수 × 3입니다.
        const std::size_t AddedVertices{static_cast<std::size_t>(Mesh.GetPolygonCount()) * 3};
        // 정점당 float 7개 크기의 정점 버퍼가 Uint32 바이트 크기를 넘지 않도록 하는 최대 정점 수입니다.
        constexpr std::size_t MaximumVertices{std::numeric_limits<Core::Uint32>::max() / (sizeof(float) * 7)};
        // 이전 메시들이 이미 추가한 정점 수입니다. 이 메시의 정점은 이 뒤에 이어 붙습니다.
        const std::size_t VertexCount{Data.GetPositions().size()};
        // UV 채널 수는 메시마다 다를 수 있으므로, 지금까지의 채널 수와 이 메시의 채널 수 중 큰 값으로 맞춥니다.
        const std::size_t UVCount{std::max(Data.GetUVChannels().size(), static_cast<std::size_t>(Mesh.GetElementUVCount()))};

        // 다음 중 하나라도 해당하면 임포트할 수 없는 메시로 판단합니다.
        // - 삼각형 메시가 아님(ImportSource에서 Triangulate를 거쳤으므로 정상이라면 항상 삼각형입니다)
        // - 컨트롤 포인트(위치 정점)가 없음
        // - 누적 정점 수가 MaximumVertices를 초과함(뺄셈으로 비교하여 덧셈 오버플로를 피합니다)
        // - 정점당 UV 데이터(채널 수 × float 2개)가 D3D11의 구조체 크기 제한(2048바이트)을 초과함
        // - 전체 UV 데이터(정점 수 × 채널 수 × float 2개)가 Uint32 바이트 크기를 초과함
        if (!Mesh.IsTriangleMesh() || Mesh.GetControlPointsCount() == 0 || AddedVertices > MaximumVertices - VertexCount || UVCount > D3D11_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES / (sizeof(float) * 2) || VertexCount + AddedVertices > std::numeric_limits<Core::Uint32>::max() / (std::max<std::size_t>(1, UVCount) * sizeof(float) * 2)) {
            Error = "The FBX mesh is invalid or exceeds mesh buffer limits";
            return false;
        }

        // 삼각형이 없는 메시는 추가할 것이 없으므로 성공으로 처리하고 건너뜁니다.
        if (AddedVertices == 0) {
            return true;
        }

        // 지오메트릭 변환은 노드 자신의 지오메트리에만 적용되고 자식 노드에는 상속되지 않는 오프셋 변환입니다.
        // EvaluateGlobalTransform에는 포함되지 않으므로 직접 행렬로 만들어 전역 변환 뒤에(먼저 적용되도록) 곱합니다.
        FbxAMatrix Geometry{};
        Geometry.SetT(Node.GetGeometricTranslation(FbxNode::eSourcePivot));
        Geometry.SetR(Node.GetGeometricRotation(FbxNode::eSourcePivot));
        Geometry.SetS(Node.GetGeometricScaling(FbxNode::eSourcePivot));
        // 메시 로컬 공간의 정점을 Scene 전역 공간으로 옮기는 최종 변환입니다. 애니메이션은 끄므로 시간 0의 변환을 사용합니다.
        const FbxAMatrix Transform{Node.EvaluateGlobalTransform(FBXSDK_TIME_ZERO) * Geometry};
        const double Determinant{FbxMatrix{Transform}.Determinant()};

        // 행렬식이 0에 가까우면 스케일이 0인 축이 있어 역행렬(노멀 변환에 필요)을 구할 수 없으므로 실패시킵니다.
        if (!std::isfinite(Determinant) || std::abs(Determinant) < 1.0e-12) {
            Error = "The FBX mesh has a non-invertible node transform";
            return false;
        }

        // 노멀은 비균일 스케일에서도 표면에 수직을 유지하도록 변환 행렬의 역전치 행렬로 변환합니다.
        const FbxAMatrix NormalTransform{Transform.Inverse().Transpose()};
        // 행렬식이 음수면 음수 스케일 등으로 메시가 거울처럼 뒤집혀 삼각형 감김 순서가 반대가 됩니다.
        const bool Mirrored{Determinant < 0.0};

        // 노멀이 없는 메시는 SDK로 노멀을 생성합니다. 노멀은 필수이므로 생성에 실패하면 임포트를 실패시킵니다.
        if (Mesh.GetElementNormalCount() == 0 && !Mesh.GenerateNormals()) {
            Error = "Cannot generate FBX mesh normals";
            return false;
        }

        // UV가 있는데 탄젠트가 없으면 첫 번째 UV 채널(0)을 기준으로 탄젠트·바이탄젠트를 생성합니다.
        // 두 번째 인자 false는 기존 데이터를 덮어쓰지 않는다는 뜻입니다. 실패해도 아래에서 임의의 탄젠트로 대체하므로 결과를 확인하지 않습니다.
        if (Mesh.GetElementUVCount() > 0 && Mesh.GetElementTangentCount() == 0) {
            Mesh.GenerateTangentsData(0, false);
        }

        // 노드의 로컬 재질 인덱스(0..GetMaterialCount()-1)를 임포트 결과의 전역 재질 인덱스로 바꾸는 표입니다.
        Core::TArray<Core::Uint32> NodeMaterials{};
        // FBX 재질의 전역 인덱스를 찾아(처음이면 읽어서) NodeMaterials 끝에 추가하는 도우미입니다.
        const auto AddMaterial{[&](FbxSurfaceMaterial* Material) {
            Core::Uint32 MaterialIndex{};

            if (!FindMaterialIndex(Material, MaterialIndex)) {
                return false;
            }

            NodeMaterials.push_back(MaterialIndex);
            return true;
        }};

        // 노드의 재질을 순서대로 등록합니다. 재질이 하나도 없으면 기본 재질(nullptr) 하나를 로컬 인덱스 0으로 등록합니다.
        for (int Index{}; Index < std::max(1, Node.GetMaterialCount()); ++Index) {
            if (!AddMaterial(Node.GetMaterialCount() == 0 ? nullptr : Node.GetMaterial(Index))) {
                return false;
            }
        }

        // 이 메시가 추가할 정점 수만큼 미리 메모리를 확보합니다.
        Data.GetPositions().reserve(VertexCount + AddedVertices);
        Data.GetNormals().reserve(VertexCount + AddedVertices);
        Data.GetTangents().reserve(VertexCount + AddedVertices);
        Data.GetColors().reserve(VertexCount + AddedVertices);
        // 이 메시 때문에 UV 채널 수가 늘었다면 새 채널을 추가합니다.
        Data.GetUVChannels().resize(UVCount);

        // 모든 UV 채널의 길이를 정점 수와 맞춥니다. 새로 생긴 채널은 이전 메시 정점 몫을 (0, 0)으로 채웁니다.
        for (auto& Channel : Data.GetUVChannels()) {
            Channel.resize(VertexCount);
            Channel.reserve(VertexCount + AddedVertices);
        }

        // 로컬 재질 인덱스별로 그 재질을 쓰는 삼각형의 정점 인덱스를 모아 두는 목록입니다. 나중에 재질별 서브메시가 됩니다.
        Core::TArray<Core::TArray<Core::Uint32>> Sections{};
        Sections.resize(NodeMaterials.size());

        for (int Polygon{}; Polygon < Mesh.GetPolygonCount(); ++Polygon) {
            // 이 삼각형이 사용하는 로컬 재질 인덱스입니다. 재질 매핑 정보가 없으면 0번 재질을 사용합니다.
            int LocalMaterial{};

            // 노드에 재질이 있고 메시에 재질 매핑 요소가 있으면, 매핑 모드에 따라 이 삼각형의 재질 인덱스를 읽습니다.
            if (const auto* Element{Mesh.GetElementMaterial()}; Element != nullptr && Node.GetMaterialCount() > 0) {
                // eAllSame은 모든 삼각형이 인덱스 배열의 0번 항목을 공유하고, eByPolygon은 삼각형마다 자기 항목을 가집니다.
                const int MaterialEntry{Element->GetMappingMode() == FbxLayerElement::eAllSame ? 0 : Polygon};

                // 재질은 위 두 매핑 모드만 의미가 있으므로 다른 모드이거나 인덱스 배열 범위를 벗어나면 실패시킵니다.
                if ((Element->GetMappingMode() != FbxLayerElement::eAllSame && Element->GetMappingMode() != FbxLayerElement::eByPolygon) || MaterialEntry >= Element->GetIndexArray().GetCount()) {
                    Error = "The FBX mesh contains an invalid material mapping";
                    return false;
                }

                LocalMaterial = Element->GetIndexArray().GetAt(MaterialEntry);
            }

            // -1은 "재질 미지정"을 뜻하는 FBX 값이므로 허용하고, 그 외에 노드 재질 범위를 벗어난 값은 실패시킵니다.
            if (LocalMaterial < -1 || LocalMaterial >= std::max(1, Node.GetMaterialCount())) {
                Error = "The FBX mesh references an invalid material slot";
                return false;
            }

            // 재질이 지정되지 않은 삼각형은 노드 재질 바로 뒤의 추가 슬롯(로컬 인덱스 = GetMaterialCount())에 모읍니다.
            if (LocalMaterial == -1) {
                LocalMaterial = Node.GetMaterialCount();

                // 추가 슬롯이 아직 없으면(처음 만난 미지정 삼각형) 기본 재질을 등록하고 그 슬롯의 Section도 만듭니다.
                if (NodeMaterials.size() == static_cast<std::size_t>(LocalMaterial)) {
                    if (!AddMaterial(nullptr)) {
                        return false;
                    }

                    Sections.emplace_back();
                }
            }

            // 삼각형의 세 모서리를 하나씩 정점으로 만듭니다.
            for (int Corner{}; Corner < 3; ++Corner) {
                // 거울 변환된 메시는 감김 순서가 뒤집히므로, 모서리를 역순(2, 1, 0)으로 읽어 원래 앞면 방향을 되돌립니다.
                const int SourceCorner{Mirrored ? 2 - Corner : Corner};
                // 이 모서리가 가리키는 컨트롤 포인트(위치 정점) 인덱스입니다. 여러 삼각형이 같은 컨트롤 포인트를 공유할 수 있습니다.
                const int ControlPoint{Mesh.GetPolygonVertex(Polygon, SourceCorner)};
                // 메시 전체에서 이 모서리의 순번(폴리곤 정점 인덱스)입니다. eByPolygonVertex 매핑 속성을 읽을 때 사용합니다.
                const int PolygonVertex{Mesh.GetPolygonVertexIndex(Polygon) + SourceCorner};
                // 노멀·탄젠트·색상·UV 같은 레이어 요소를 매핑 모드에 맞는 인덱스로 읽는 도우미입니다.
                // 요소가 없으면 Value를 바꾸지 않고 true를 반환하므로, 아래에서 지정한 기본값이 그대로 사용됩니다.
                const auto Read{[&](const auto* Element, auto& Value) {
                    return ReadElement(Element, ControlPoint, Polygon, PolygonVertex, Value);
                }};

                if (ControlPoint < 0 || ControlPoint >= Mesh.GetControlPointsCount()) {
                    Error = "The FBX mesh contains an invalid control point";
                    return false;
                }

                // 위치는 이동을 포함한 전체 변환으로 전역 공간에 옮긴 뒤, 왼손 좌표계로 바꾸기 위해 Y를 반전합니다.
                const FbxVector4 Position{Transform.MultT(Mesh.GetControlPointAt(ControlPoint))};
                const Core::FVector Vertex{static_cast<float>(Position[0]), -static_cast<float>(Position[1]), static_cast<float>(Position[2])};
                // 요소가 없을 때의 기본값입니다. 탄젠트·바이탄젠트는 0 벡터, 정점 색상은 흰색입니다.
                FbxVector4 SourceNormal{};
                FbxVector4 SourceTangent{};
                FbxVector4 SourceBitangent{};
                FbxColor Color{1.0, 1.0, 1.0, 1.0};

                if (!Read(Mesh.GetElementNormal(), SourceNormal) || !Read(Mesh.GetElementTangent(), SourceTangent) || !Read(Mesh.GetElementBinormal(), SourceBitangent) || !Read(Mesh.GetElementVertexColor(), Color)) {
                    Error = "The FBX mesh contains an invalid vertex attribute mapping";
                    return false;
                }

                // 노멀은 역전치 행렬로 변환하고(Y 반전 포함) 단위 길이로 정규화합니다. 길이가 0이면 정규화에 실패합니다.
                Core::FVector Normal{TransformDirection(NormalTransform, SourceNormal)};

                if (!Normal.IsFinite() || !Normal.Normalize()) {
                    Error = "The FBX mesh contains an invalid normal";
                    return false;
                }

                // 탄젠트와 바이탄젠트는 표면을 따라 놓인 방향이므로 노멀과 달리 원래 변환 행렬로 변환합니다.
                const Core::FVector TangentDirection{TransformDirection(Transform, SourceTangent)};
                const Core::FVector Bitangent{TransformDirection(Transform, SourceBitangent)};
                // 그람-슈미트 직교화: 탄젠트에서 노멀 방향 성분을 빼서 노멀과 정확히 수직이 되도록 만듭니다.
                Core::FVector Tangent{TangentDirection - Normal * Normal.Dot(TangentDirection)};
                // 바이탄젠트 = Handedness × (Normal × Tangent)로 복원할 때 쓰는 부호(±1)입니다. 탄젠트의 w 성분으로 저장됩니다.
                float Handedness{1.0f};

                // 탄젠트 데이터가 있는데 변환 결과가 유효하지 않으면 실패시킵니다.
                if (Mesh.GetElementTangent() != nullptr && (!Tangent.IsFinite() || !Bitangent.IsFinite())) {
                    Error = "The FBX mesh contains an invalid tangent";
                    return false;
                }

                if (!Tangent.Normalize()) {
                    // 탄젠트가 없거나(0 벡터) 노멀과 평행해 직교화 후 길이가 0이면, 노멀에 수직인 임의의 탄젠트를 만듭니다.
                    // 노멀과 거의 평행하지 않은 기준 축(기본 Z, 노멀이 Z에 가까우면 Y)과 노멀의 외적을 사용합니다.
                    const Core::FVector Axis{std::abs(Normal.z) < 0.99f ? Core::FVector{0.0f, 0.0f, 1.0f} : Core::FVector{0.0f, 1.0f, 0.0f}};
                    Tangent = Axis.Cross(Normal).Normalized();
                } else if (Mesh.GetElementBinormal() != nullptr) {
                    // 바이탄젠트가 있으면 (Normal × Tangent)와 실제 바이탄젠트가 같은 방향인지로 부호를 정합니다.
                    // 위치·방향이 Y 반전(그리고 거울 변환)을 거쳐 외적의 방향이 뒤집힌 공간이므로, 양수일 때 -1이 원본 FBX 기준의 부호에 해당합니다.
                    Handedness = Normal.Cross(Tangent).Dot(Bitangent) > 0.0f ? -1.0f : 1.0f;
                } else {
                    // 바이탄젠트가 없으면 FBX 탄젠트의 w 성분에 저장된 부호를 사용합니다.
                    // 이 값은 노드 변환을 거치지 않았으므로, 거울 변환된 메시는 직접 부호를 반전합니다.
                    Handedness = static_cast<float>(SourceTangent[3]) * (Mirrored ? -1.0f : 1.0f);
                    // w가 0이거나 ±1이 아닌 값이어도 정확히 ±1이 되도록 정리합니다.
                    Handedness = Handedness < 0.0f ? -1.0f : 1.0f;
                }

                const Core::FVector4 VertexColor{static_cast<float>(Color.mRed), static_cast<float>(Color.mGreen), static_cast<float>(Color.mBlue), static_cast<float>(Color.mAlpha)};

                if (!Vertex.IsFinite() || !VertexColor.IsFinite()) {
                    Error = "The FBX mesh contains invalid vertex attributes";
                    return false;
                }

                // 새 정점의 인덱스(추가 직전의 정점 수)를 이 삼각형 재질의 Section에 기록하고, 정점 속성을 각 스트림에 추가합니다.
                Sections[LocalMaterial].push_back(static_cast<Core::Uint32>(Data.GetPositions().size()));
                Data.GetPositions().push_back(Vertex);
                Data.GetNormals().push_back(Normal);
                Data.GetTangents().push_back({Tangent, Handedness});
                Data.GetColors().push_back(VertexColor);

                // 모든 UV 채널에 이 정점의 좌표를 하나씩 추가하여 채널 길이를 정점 수와 맞춥니다.
                for (std::size_t Channel{}; Channel < UVCount; ++Channel) {
                    // 다른 메시 때문에 생긴 채널이라 이 메시에는 없는 채널일 수 있습니다. 그 경우 (0, 0)을 넣습니다.
                    const bool HasChannel{Channel < static_cast<std::size_t>(Mesh.GetElementUVCount())};
                    FbxVector2 UV{};

                    if (HasChannel && !Read(Mesh.GetElementUV(static_cast<int>(Channel)), UV)) {
                        Error = "The FBX mesh contains an invalid UV mapping";
                        return false;
                    }

                    // FBX UV는 왼쪽 아래가 원점이고 D3D 텍스처 좌표는 왼쪽 위가 원점이므로 V를 1 - V로 뒤집습니다.
                    const Core::FVector2 Coordinates{static_cast<float>(UV[0]), HasChannel ? 1.0f - static_cast<float>(UV[1]) : 0.0f};

                    if (!Coordinates.IsFinite()) {
                        Error = "The FBX mesh contains invalid texture coordinates";
                        return false;
                    }

                    Data.GetUVChannels()[Channel].push_back(Coordinates);
                }
            }
        }

        // 재질별로 모은 정점 인덱스를 인덱스 버퍼에 이어 붙이고, 각 구간을 하나의 서브메시로 등록합니다.
        // 서브메시는 (인덱스 버퍼 시작 위치, 인덱스 수, 전역 재질 인덱스)이며, 삼각형이 없는 재질은 서브메시를 만들지 않습니다.
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
        // 노드에 메시가 붙어 있으면 먼저 읽습니다. 메시가 아닌 노드(카메라, 조명, 빈 그룹 등)는 건너뜁니다.
        if (auto* Mesh{Node.GetMesh()}; Mesh != nullptr && !ReadMesh(Node, *Mesh)) {
            return false;
        }

        // 자식 노드를 재귀적으로 방문하여 Scene 트리 전체의 메시를 읽습니다. 하나라도 실패하면 즉시 중단합니다.
        for (int Index{}; Index < Node.GetChildCount(); ++Index) {
            if (!ReadNode(*Node.GetChild(Index))) {
                return false;
            }
        }

        return true;
    }

    bool Asset::FFBXLoader::ImportSource() {
        Core::FString& Error{GetError()};
        // FBX SDK의 모든 객체를 생성·소유하는 Manager를 만듭니다. 소멸자에서 Destroy로 한 번에 해제합니다.
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

        // 언리얼 FBX 임포터의 Convert Scene이 켜지고 Force Front XAxis가 꺼진 경우와 같은 축 변환을 사용합니다.
        // 이 옵션은 임포트할 메시의 축을 정하는 정책이므로, 엔진 카메라의 Forward = +X만으로 값을 선택하지 않습니다.
        // 언리얼은 먼저 FBX Scene을 Z-Up, Front = -Y인 오른손 좌표계로 변환합니다.
        // 그다음 정점과 방향 벡터를 엔진 데이터로 읽을 때 Y를 반전하여 최종 왼손 좌표계로 옮깁니다.
        //
        // 첫 번째 인자 eZAxis는 이 중간 좌표계의 Up이 +Z라는 뜻입니다.
        // 두 번째 인자는 Up을 제외한 나머지 두 축 중 FBX Front로 사용할 축과 그 부호를 정합니다.
        // X, Y, Z 순서에서 Up을 제외했을 때 eParityEven은 첫 번째 축, eParityOdd는 두 번째 축을 선택합니다.
        // Z-Up에서는 남은 축이 X, Y이므로 eParityEven은 +X, eParityOdd는 +Y, -eParityOdd는 -Y를 의미합니다.
        // 언리얼의 Force Front XAxis가 꺼져 있으면 -eParityOdd를 사용하고, 켜져 있으면 eParityEven을 사용합니다.
        // 단항 - 연산의 결과는 int이므로 생성자가 받는 EFrontVector 타입으로 명시적으로 변환합니다.
        //
        // 세 번째 인자 eRightHanded는 최종 엔진 좌표계가 아니라 이 단계의 FBX 중간 좌표계가 오른손 좌표계라는 뜻입니다.
        // Scene은 이 단계 이후에도 오른손 좌표계이며, ReadMesh의 위치와 TransformDirection의 방향 출력에서 Y를 반전합니다.
        const FbxAxisSystem AxisSystem{FbxAxisSystem::eZAxis, static_cast<FbxAxisSystem::EFrontVector>(-FbxAxisSystem::eParityOdd), FbxAxisSystem::eRightHanded};

        // 언리얼과 같은 ConvertScene을 사용하여 노드의 변환에 축 변환을 반영합니다.
        // 이후 ReadMesh는 EvaluateGlobalTransform으로 이 변환을 포함한 정점과 방향을 계산합니다.
        // DeepConvertScene으로 왼손 좌표계까지 한 번에 변환하면 삼각형 순서 등도 달라질 수 있으므로 두 단계를 유지합니다.
        AxisSystem.ConvertScene(mScene);

        // Scene의 길이 단위를 원본 FBX에 저장된 단위에서 미터로 변환합니다. FbxSystemUnit::m은 미터 단위입니다.
        // 축 변환이 방향을 맞추는 작업이라면, 단위 변환은 동일한 실제 길이가 유지되도록 수치와 스케일을 맞추는 작업입니다.
        // 예를 들어 센티미터 단위 Scene의 100cm 길이가 변환 후에도 1m에 해당하도록 조정합니다.
        // 별도 옵션을 전달하지 않았으므로 SDK의 기본 단위 변환 옵션을 사용합니다.
        FbxSystemUnit::m.ConvertScene(mScene);

        // 같은 FBX Manager를 사용하는 지오메트리 변환 도구를 생성합니다. 생성 시점에는 메시를 변환하지 않습니다.
        // 아래 Triangulate 호출에서 Scene 내 변환 가능한 지오메트리를 삼각형으로 분할하는 데 사용합니다.
        // Triangulate의 두 번째 인자 true는 기존 지오메트리를 삼각형으로 변환한 결과로 교체하도록 지정합니다.
        FbxGeometryConverter Converter{mManager};

        if (!Converter.Triangulate(mScene, true)) {
            Error = "Cannot triangulate the FBX scene";
            return false;
        }

        // 노드 트리를 순회할 시작점인 루트 노드가 없으면 읽을 것이 없으므로 실패시킵니다.
        if (mScene->GetRootNode() == nullptr) {
            Error = "The FBX scene has no root node";
            return false;
        }

        // 루트 노드부터 Scene 트리를 재귀적으로 순회하며 모든 메시를 결과 메시 데이터에 추가합니다.
        return ReadNode(*mScene->GetRootNode());
    }

}
