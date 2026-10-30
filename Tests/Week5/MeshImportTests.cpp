/**
 * @file MeshImportTests.cpp
 * @brief 메시 임포트의 재질·텍스처·변환과 MTL 호환성을 검증합니다.
 */

#include "MeshImportTests.h"
#include "Core/Guid.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UTexture2D.h"
#include "Engine/Import/FAssimpMeshImporter.h"
#include "Engine/Import/FMeshImportResult.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

/**
 * @brief 해당 검증 파일에서만 사용하는 내부 검증 도우미입니다.
 */
namespace {
    /**
     * @brief 메시 임포트 검증 조건이 거짓이면 예외를 발생시킵니다.
     * @param[in] Condition 검증을 통과해야 하는 조건입니다.
     * @param[out] Error 실패 원인을 받을 문자열입니다.
     */
    void RequireMeshImport(bool Condition, const FString& Error) {
        if (!Condition) {
            throw std::runtime_error{Error};
        }
    }

    FString ReadImportFile(const std::filesystem::path& Path) {
        std::ifstream Input{Path, std::ios::binary};
        RequireMeshImport(static_cast<bool>(Input), "Cannot read the mesh import fixture");
        return FString{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}};
    }

    TArray<std::filesystem::path> GetImportFiles(const std::filesystem::path& Directory) {
        TArray<std::filesystem::path> Files{};
        for (const auto& Entry : std::filesystem::recursive_directory_iterator{Directory}) {
            if (Entry.is_regular_file()) {
                Files.push_back(Entry.path());
            }
        }
        std::sort(Files.begin(), Files.end());
        return Files;
    }

    void TestMeshImportResult(const std::filesystem::path& Source) {
        FString Error{"Previous import error"};
        FMeshImportResult Imported{};
        {
            const FAssimpMeshImporter Assimp{};
            const IMeshImporter& Importer{Assimp};
            RequireMeshImport(Importer.Import(Source / L"메시.obj", Imported, Error), Error);
        }
        RequireMeshImport(Error.empty(), "A successful mesh import retained an old error");
        const FMeshData& Data{Imported.GetMeshData()};
        RequireMeshImport(Data.GetPositions().size() == 3 && Data.GetIndices().size() == 3 && Data.GetSubMeshes().size() == 1, "The common import result lost its geometry");
        RequireMeshImport(Data.GetMaterialSlots().size() == Imported.GetMaterials().size(), "Imported materials do not share the mesh slot indices");
        const Uint32 Slot{Data.GetSubMeshes().front().GetMaterialIndex()};
        const FMeshImportMaterial& Material{Imported.GetMaterials()[Slot]};
        RequireMeshImport(Material.GetName() == "Surface" && Material.GetName() == Data.GetMaterialSlots()[Slot].GetName() && Material.GetBaseColor() == FVector4{0.25f, 0.5f, 0.75f, 0.5f}, "The common import result lost its material values");
        RequireMeshImport(Data.GetMaterialSlots()[Slot].GetMaterialPath().empty(), "A source import result contains a project asset path");
        RequireMeshImport(Material.GetBaseColorTexture().GetSource() == EMeshImportTextureSource::ExternalFile && Material.GetBaseColorTexture().GetFilePath() == Source / "Color.tga" && Material.GetNormalMapTexture().GetFilePath() == Source / "Normal.tga", "Imported texture paths are not resolved independently of Assimp");
        const FMeshImportResult Previous{Imported};
        RequireMeshImport(!IMeshImporter::GetDefault().Import(Source / "Missing.obj", Imported, Error) && !Error.empty(), "A missing mesh source was accepted");
        RequireMeshImport(Imported.GetMeshData().GetPositions() == Previous.GetMeshData().GetPositions() && Imported.GetMaterials()[Slot].GetBaseColorTexture().GetFilePath() == Previous.GetMaterials()[Slot].GetBaseColorTexture().GetFilePath(), "A failed mesh import replaced the previous result");
        RequireMeshImport(!IMeshImporter::GetDefault().Import({}, Imported, Error) && !Error.empty(), "An empty mesh source path was accepted");
    }

    void TestMeshImportSections(const std::filesystem::path& Directory, const std::filesystem::path& Source) {
        {
            std::ofstream Output{Source / "Sections.mtl", std::ios::binary};
            Output << "newmtl Red\nKd 1 0 0\nmap_Kd Color.tga\nnewmtl Blue\nKd 0 0 1\nmap_Kd Color.tga\nnorm Color.tga\n";
            RequireMeshImport(static_cast<bool>(Output), "Cannot write the multi-material fixture");
        }
        {
            std::ofstream Output{Source / "Sections.obj", std::ios::binary};
            Output << "mtllib Sections.mtl\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Red\nf 1/1 2/2 3/3\nusemtl Blue\nf 1/1 3/3 4/4\n";
            RequireMeshImport(static_cast<bool>(Output), "Cannot write the multi-section fixture");
        }
        FString Error{};
        FMeshImportResult Imported{};
        RequireMeshImport(IMeshImporter::GetDefault().Import(Source / "Sections.obj", Imported, Error), Error);
        RequireMeshImport(Imported.GetMeshData().GetSubMeshes().size() == 2 && Imported.GetMeshData().GetIndices().size() == 6, "Multi-material geometry was merged incorrectly");
        bool SeenRed{};
        bool SeenBlue{};
        for (const FSubMesh& Section : Imported.GetMeshData().GetSubMeshes()) {
            const auto& Material{Imported.GetMaterials()[Section.GetMaterialIndex()]};
            SeenRed = SeenRed || (Material.GetName() == "Red" && Material.GetBaseColor() == FVector4{1.0f, 0.0f, 0.0f, 1.0f});
            SeenBlue = SeenBlue || (Material.GetName() == "Blue" && Material.GetBaseColor() == FVector4{0.0f, 0.0f, 1.0f, 1.0f});
        }
        RequireMeshImport(SeenRed && SeenBlue, "Mesh sections do not reference the matching imported materials");
        FAssetManager Assets{Directory / "SectionContent"};
        RequireMeshImport(Assets.ImportMesh(Source / "Sections.obj", "Sections.masset", Error), Error);
        FAssetManager Reloaded{Directory / "SectionContent"};
        const UStaticMesh* Mesh{Reloaded.LoadMesh("Sections.masset", Error)};
        RequireMeshImport(Mesh != nullptr && Mesh->GetMeshData().GetSubMeshes().size() == 2, "Saved mesh sections were lost");
        FString SharedTexture{};
        for (const auto& Section : Mesh->GetMeshData().GetSubMeshes()) {
            const auto& Slot{Mesh->GetMeshData().GetMaterialSlots()[Section.GetMaterialIndex()]};
            UMaterial* Material{Reloaded.LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
            RequireMeshImport(Material != nullptr && Material->GetBaseColorTexture() != nullptr, Error);
            if (SharedTexture.empty()) {
                SharedTexture = Material->GetBaseColorTexturePath();
            }
            RequireMeshImport(SharedTexture == Material->GetBaseColorTexturePath(), "Shared source textures were imported more than once");
            if (Material->GetName() == "Blue") {
                RequireMeshImport(Material->GetNormalMapTexturePath() == SharedTexture && Material->GetNormalMapTexture() != nullptr && !Material->GetNormalMapTexture()->IsSRGB(), "A shared texture lost its normal-map interpretation");
            }
        }
        const auto DependencyDirectory{Assets.GetContentRoot() / ".Imported" / "Sections"};
        const auto Files{GetImportFiles(DependencyDirectory)};
        RequireMeshImport(std::count_if(Files.begin(), Files.end(), [](const auto& Path) {
            return Path.extension() == L".masset";
        }) == 1, "Texture deduplication created extra texture assets");
        const FString PreviousAsset{ReadImportFile(Assets.GetContentRoot() / "Sections.masset")};
        {
            std::ofstream Output{Source / "Sections.mtl", std::ios::binary};
            Output << "newmtl Red\nKd 1 0 0\nmap_Kd Color.tga\nnewmtl Blue\nKd 0 0 1\nmap_Kd Missing.tga\n";
            RequireMeshImport(static_cast<bool>(Output), "Cannot write the missing-texture fixture");
        }
        RequireMeshImport(!Assets.ImportMesh(Source / "Sections.obj", "Sections.masset", Error) && Error.find("Missing.tga") != FString::npos, "A missing material texture was accepted");
        RequireMeshImport(ReadImportFile(Assets.GetContentRoot() / "Sections.masset") == PreviousAsset && GetImportFiles(DependencyDirectory) == Files, "A failed import changed the saved mesh or left dependency files");
    }

    void TestMeshImportGltf(const std::filesystem::path& Directory, const std::filesystem::path& Source) {
        const std::array<float, 33> Attributes{0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.2f, 0.3f, 0.8f, 0.3f, 0.2f, 0.9f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f};
        const std::array<std::uint16_t, 3> Indices{0, 1, 2};
        {
            std::ofstream Output{Source / "Triangle.bin", std::ios::binary};
            Output.write(reinterpret_cast<const char*>(Attributes.data()), sizeof(Attributes));
            Output.write(reinterpret_cast<const char*>(Indices.data()), sizeof(Indices));
            RequireMeshImport(static_cast<bool>(Output), "Cannot write the glTF geometry fixture");
        }
        {
            std::ofstream Output{Source / "Triangle.gltf", std::ios::binary};
            Output << R"({"asset":{"version":"2.0"},"buffers":[{"uri":"Triangle.bin","byteLength":138}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":24},{"buffer":0,"byteOffset":60,"byteLength":24},{"buffer":0,"byteOffset":84,"byteLength":48},{"buffer":0,"byteOffset":132,"byteLength":6}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},{"bufferView":1,"componentType":5126,"count":3,"type":"VEC2"},{"bufferView":2,"componentType":5126,"count":3,"type":"VEC2"},{"bufferView":3,"componentType":5126,"count":3,"type":"VEC4"},{"bufferView":4,"componentType":5123,"count":3,"type":"SCALAR"}],"images":[{"uri":"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+jRZkAAAAASUVORK5CYII="}],"textures":[{"source":0}],"materials":[{"name":"Embedded","doubleSided":true,"pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],"meshes":[{"primitives":[{"attributes":{"POSITION":0,"TEXCOORD_0":1,"TEXCOORD_1":2,"COLOR_0":3},"indices":4,"material":0}]}],"nodes":[{"mesh":0,"translation":[2,3,4],"scale":[2,1,1]}],"scenes":[{"nodes":[0]}],"scene":0})";
            RequireMeshImport(static_cast<bool>(Output), "Cannot write the glTF scene fixture");
        }
        FString Error{};
        FMeshImportResult Imported{};
        RequireMeshImport(IMeshImporter::GetDefault().Import(Source / "Triangle.gltf", Imported, Error), Error);
        const FMeshData& Data{Imported.GetMeshData()};
        RequireMeshImport(Data.GetPositions().size() == 3 && Data.GetUVChannels().size() == 2 && Data.GetColors().size() == 3 && Data.GetNormals().size() == 3 && Data.GetTangents().size() == 3, "The import result lost glTF vertex attributes");
        RequireMeshImport(Data.GetBounds().mMinimum == FVector{2.0f, 3.0f, -4.0f} && Data.GetBounds().mMaximum == FVector{4.0f, 4.0f, -4.0f}, "Assimp node transforms or left-handed conversion changed");
        const auto& Material{Imported.GetMaterials()[Data.GetSubMeshes().front().GetMaterialIndex()]};
        RequireMeshImport(Material.IsTwoSided() && Material.GetBaseColorTexture().GetSource() == EMeshImportTextureSource::Embedded && !Material.GetBaseColorTexture().GetEmbeddedReference().empty(), "Embedded textures leaked an Assimp-specific lookup requirement");
        UStaticMesh Mesh{};
        RequireMeshImport(Mesh.LoadFromFile(Source / "Triangle.gltf", Error), Error);
        FAssetManager Assets{Directory / "EmbeddedContent"};
        RequireMeshImport(!Assets.ImportMesh(Source / "Triangle.gltf", "Embedded.masset", Error) && Error.find("Embedded") != FString::npos && !std::filesystem::exists(Assets.GetContentRoot() / "Embedded.masset"), "Asset import accepted an unsupported embedded texture");
    }
}

/**
 * @brief 메시와 MTL·텍스처 임포트 결과를 검증합니다.
 */
void TestMeshImport() {
    const std::filesystem::path Directory{std::filesystem::absolute("Intermediate/MeshImportTests") / FilePath::FromUtf8(FGuid{}.ToString())};
    const std::filesystem::path Source{Directory / L"한글 경로"};
    std::filesystem::create_directories(Source);
    const FString MaterialText{"newmtl Surface\r\nPl 1\r\nPr 0.25\r\n\tPrfl 1\r\nKd 0.25 0.5 0.75\r\nd 0.5\r\nmap_Kd Color.tga\r\nnorm Normal.tga\r\n"};
    {
        std::ofstream Output{Source / "Surface.MTL", std::ios::binary};
        Output << MaterialText;
        RequireMeshImport(static_cast<bool>(Output), "Cannot write mesh import material fixture");
    }
    {
        std::ofstream Output{Source / L"메시.obj", std::ios::binary};
        Output << "mtllib Surface.MTL\nv 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nusemtl Surface\nf 1/1 2/2 3/3\n";
        RequireMeshImport(static_cast<bool>(Output), "Cannot write mesh import geometry fixture");
    }
    const std::array<Uint8, 22> Image{0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 32, 40, 255, 128, 128, 255};
    for (const char* Name : {"Color.tga", "Normal.tga"}) {
        std::ofstream Output{Source / Name, std::ios::binary};
        Output.write(reinterpret_cast<const char*>(Image.data()), static_cast<std::streamsize>(Image.size()));
        RequireMeshImport(static_cast<bool>(Output), "Cannot write mesh import texture fixture");
    }
    TestMeshImportResult(Source);
    TestMeshImportSections(Directory, Source);
    TestMeshImportGltf(Directory, Source);
    FString Error{};
    UStaticMesh DirectMesh{};
    RequireMeshImport(DirectMesh.LoadFromFile(Source / L"메시.obj", Error), Error);
    FAssetManager Assets{Directory / "Content"};
    RequireMeshImport(Assets.ImportMesh(Source / L"메시.obj", "Imported.masset", Error), Error);
    FAssetManager ReloadedAssets{Directory / "Content"};
    const UStaticMesh* Mesh{ReloadedAssets.LoadMesh("Imported.masset", Error)};
    RequireMeshImport(Mesh != nullptr, Error);
    RequireMeshImport(Mesh->GetVertexCount() == 3 && Mesh->GetIndexCount() == 3 && Mesh->GetMeshData().GetSubMeshes().size() == 1, "Imported mesh geometry did not survive saving and reloading");
    const auto& Slot{Mesh->GetMeshData().GetMaterialSlots()[Mesh->GetMeshData().GetSubMeshes().front().GetMaterialIndex()]};
    UMaterial* Material{ReloadedAssets.LoadMaterial(FilePath::FromUtf8(Slot.GetMaterialPath()), Error)};
    RequireMeshImport(Material != nullptr, Error);
    RequireMeshImport(Material->GetName() == "Surface" && Material->GetBaseColor() == FVector4{0.25f, 0.5f, 0.75f, 0.5f}, "Imported material name or color was lost");
    RequireMeshImport(Material->GetBaseColorTexture() != nullptr && Material->GetNormalMapTexture() != nullptr && !Material->GetNormalMapTexture()->IsSRGB(), "Imported material textures were lost");
    {
        std::ifstream Input{Source / "Surface.MTL", std::ios::binary};
        const FString Unchanged{std::istreambuf_iterator<char>{Input}, std::istreambuf_iterator<char>{}};
        RequireMeshImport(Unchanged == MaterialText, "Mesh import changed the source material file");
    }
    RequireMeshImport(!Assets.ImportMesh(Source / "Missing.obj", "Missing.masset", Error) && Error.find("Missing.obj") != FString::npos && Error != "Cannot import mesh source", "Mesh import did not report its source path and loader error");
    {
        std::ofstream Output{Source / "Surface.MTL", std::ios::binary};
        Output << "newmtl Surface\nPr invalid\n";
        RequireMeshImport(static_cast<bool>(Output), "Cannot write invalid mesh import material fixture");
    }
    RequireMeshImport(!Assets.ImportMesh(Source / L"메시.obj", "Invalid.masset", Error) && Error.find("invalid") != FString::npos, "Mesh import ignored a malformed supported material value");
}
