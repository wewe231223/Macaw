#include "MeshImportTests.h"
#include "Core/Guid.h"
#include "Core/FilePath.h"
#include "Engine/FAssetManager.h"
#include "Engine/UMaterial.h"
#include "Engine/UStaticMesh.h"
#include "Engine/UTexture2D.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
    void RequireMeshImport(bool Condition, const FString& Error) {
        if (!Condition) {
            throw std::runtime_error{Error};
        }
    }
}

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
