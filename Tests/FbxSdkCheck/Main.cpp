#include <fbxsdk.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace fbxsdk;

namespace {
    void DestroyManager(FbxManager* Manager) {
        Manager->Destroy();
    }

    void Check(bool Condition, const char* Error) {
        if (!Condition) {
            throw std::runtime_error{Error};
        }
    }

    void CheckRoundTrip(FbxManager& Manager, bool Ascii) {
        const auto Directory{std::filesystem::path{"Intermediate/FbxSdkCheck"}};
        std::filesystem::create_directories(Directory);
        const auto Path{Directory / (Ascii ? "TriangleAscii.fbx" : "TriangleBinary.fbx")};
        FbxScene* Source{FbxScene::Create(&Manager, "Source")};
        Check(Source != nullptr, "Cannot create the source scene");
        FbxMesh* Mesh{FbxMesh::Create(&Manager, "Triangle")};
        Check(Mesh != nullptr, "Cannot create the source mesh");
        Mesh->InitControlPoints(3);
        Mesh->SetControlPointAt(FbxVector4{0.0, 0.0, 0.0}, 0);
        Mesh->SetControlPointAt(FbxVector4{1.0, 0.0, 0.0}, 1);
        Mesh->SetControlPointAt(FbxVector4{0.0, 1.0, 0.0}, 2);
        Mesh->BeginPolygon();
        Mesh->AddPolygon(0);
        Mesh->AddPolygon(1);
        Mesh->AddPolygon(2);
        Mesh->EndPolygon();
        FbxNode* Node{FbxNode::Create(&Manager, "Triangle")};
        Check(Node != nullptr, "Cannot create the source node");
        Node->SetNodeAttribute(Mesh);
        Source->GetRootNode()->AddChild(Node);

        const char* FormatName{Ascii ? "FBX ascii (*.fbx)" : "FBX binary (*.fbx)"};
        const int Format{Manager.GetIOPluginRegistry()->FindWriterIDByDescription(FormatName)};
        Check(Format >= 0, "Cannot find the FBX writer");
        FbxExporter* Exporter{FbxExporter::Create(&Manager, "Exporter")};
        Check(Exporter != nullptr, "Cannot create the FBX exporter");
        if (!Exporter->Initialize(Path.string().c_str(), Format, Manager.GetIOSettings()) || !Exporter->Export(Source)) {
            throw std::runtime_error{Exporter->GetStatus().GetErrorString()};
        }
        Exporter->Destroy();
        Source->Destroy();

        FbxImporter* Importer{FbxImporter::Create(&Manager, "Importer")};
        Check(Importer != nullptr, "Cannot create the FBX importer");
        if (!Importer->Initialize(Path.string().c_str(), -1, Manager.GetIOSettings())) {
            throw std::runtime_error{Importer->GetStatus().GetErrorString()};
        }
        Check(Importer->IsFBX(), "The exported file is not recognized as FBX");
        FbxScene* Imported{FbxScene::Create(&Manager, "Imported")};
        Check(Imported != nullptr, "Cannot create the imported scene");
        if (!Importer->Import(Imported)) {
            throw std::runtime_error{Importer->GetStatus().GetErrorString()};
        }
        Importer->Destroy();
        Check(Imported->GetRootNode()->GetChildCount() == 1, "The imported node count is incorrect");
        FbxMesh* ImportedMesh{Imported->GetRootNode()->GetChild(0)->GetMesh()};
        Check(ImportedMesh != nullptr, "The imported mesh is missing");
        Check(ImportedMesh->GetControlPointsCount() == 3 && ImportedMesh->GetPolygonCount() == 1 && ImportedMesh->GetPolygonSize(0) == 3, "The imported triangle is incorrect");
        Check(ImportedMesh->GetControlPointAt(1)[0] == 1.0 && ImportedMesh->GetControlPointAt(2)[1] == 1.0, "The imported vertex positions are incorrect");
        Imported->Destroy();
        std::filesystem::remove(Path);
    }
}

int main() {
    try {
        const std::unique_ptr<FbxManager, decltype(&DestroyManager)> Manager{FbxManager::Create(), &DestroyManager};
        Check(Manager != nullptr, "Cannot create the FBX SDK manager");
        FbxIOSettings* Settings{FbxIOSettings::Create(Manager.get(), IOSROOT)};
        Check(Settings != nullptr, "Cannot create FBX IO settings");
        Manager->SetIOSettings(Settings);
        CheckRoundTrip(*Manager, false);
        CheckRoundTrip(*Manager, true);
        std::cout << "FBX SDK " << FbxManager::GetVersion() << ": binary and ASCII export/import passed\n";
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
}
