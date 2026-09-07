#include "PCH.h"
#include "UWorld.h"

#include <algorithm>

#include "AActor.h"
#include "Component/UCameraComponent.h"
#include "Component/UStaticMeshComponent.h"

#include "../Serialize/FArchiveJson.h"
#include "../Core/Base/TypeRegistry.h"
#include "../Core/Base/UObjectSystem.h"
#include "../Core/Asset/FAssetRegistry.h"

#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

UWorld::~UWorld()
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        UObjectSystem::Unregister(Actor.get(), Actor->GetHandle());
    }

    Actors.clear();
}

const std::vector<std::unique_ptr<AActor>>& UWorld::GetActors() const
{
    return Actors;
}

FRenderProbe& UWorld::BuildRenderProbe() 
{
	Probe.ActorProbes.clear();

    for (const UStaticMeshComponent* Component : RenderableComponents)
    {
        Component->MakeRender(Probe);
    }

    if (Camera != nullptr)
    {
        Probe.MainCameraProbe.View =
            Camera->GetViewMatrix();

        Probe.MainCameraProbe.Projection =
            Camera->GetProjectionMatrix();

        Probe.MainCameraProbe.ViewProjection =
            Camera->GetViewProjectionMatrix();
    }
    return Probe;
}

void UWorld::Tick(float DeltaTime)
{
    for (const std::unique_ptr<AActor>& Actor : Actors)
    {
        Actor->Tick(DeltaTime);
    }
}

void UWorld::RegisterRenderable(UStaticMeshComponent* Component)
{
    if (Component == nullptr || std::ranges::find(RenderableComponents, Component) != RenderableComponents.end())
    {
        return;
    }

    RenderableComponents.push_back(Component);
}

void UWorld::UnregisterRenderable(UStaticMeshComponent* Component)
{
    std::erase(RenderableComponents, Component);
}

void UWorld::SetMainCamera(UCameraComponent* InCamera)
{
    Camera = InCamera;
}

void UWorld::ClearMainCamera(UCameraComponent* InCamera)
{
    if (Camera == InCamera)
    {
        Camera = nullptr;
    }
}


bool UWorld::SaveScene(const FString& SceneName, FAssetRegistry* AssetRegistry)
{
    std::filesystem::path CurrentPath = std::filesystem::current_path();
    std::filesystem::path SceneDir = CurrentPath / "scenes";
    if (!std::filesystem::exists(SceneDir))
        std::filesystem::create_directories(SceneDir);
    std::filesystem::path FilePath = SceneDir / (SceneName.c_str() + std::string(".json"));

    rapidjson::Document Document;
    Document.SetObject();
    rapidjson::Document::AllocatorType& Allocator = Document.GetAllocator();


    FArchiveJson ArchiveSave(Document, Allocator);
	ArchiveSave.SetAssetRegistry(AssetRegistry);

    // ***** TODO function is not developed yet
	auto AssetList = AssetRegistry->GetAssetList();
    // TArray<std::unique_ptr<UObject>> AssetList;

    size_t ArraySize = static_cast<size_t>(AssetList.size());
    ArchiveSave.BeginArrayScope("Assets", ArraySize);

    for (size_t Index : std::views::iota(size_t{ 0 }, std::ranges::size(AssetList))) {
        UObject* Asset = AssetList[Index];

        ArchiveSave.BeginObjectScope(std::to_string(Index));
        Asset->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }


    //for (size_t CurrentIndex = 0, EndIndex = AssetList.size(); CurrentIndex < EndIndex; ++CurrentIndex)
    //{
    //    ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
    //    AssetList[CurrentIndex]->Save(ArchiveSave);
    //    ArchiveSave.EndObjectScope();
    //}
    ArchiveSave.EndArrayScope();

    ArraySize = static_cast<size_t>(Actors.size());
    ArchiveSave.BeginArrayScope("Actors", ArraySize);
    for (size_t CurrentIndex = 0, EndIndex = Actors.size(); CurrentIndex < EndIndex; ++CurrentIndex)
    {
        ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
        Actors[CurrentIndex]->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }
    ArchiveSave.EndArrayScope();

    std::ofstream OutputFileStream(FilePath);
    if (!OutputFileStream.is_open())
        return false;

    rapidjson::OStreamWrapper StreamWrapper(OutputFileStream);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> Writer(StreamWrapper);
    Document.Accept(Writer);
    OutputFileStream.close();

    return true;
}

bool UWorld::LoadScene(const std::filesystem::path& ScenePath, ID3D11Device* Device, FAssetRegistry* AssetRegistry)
{
    std::ifstream InputFileStream(ScenePath);
    if (!InputFileStream.is_open())
        return false;

    std::stringstream Buffer;
    Buffer << InputFileStream.rdbuf();
    std::string LoadedJsonString = Buffer.str();
    InputFileStream.close();

    rapidjson::Document LoadDocument;
    LoadDocument.Parse(LoadedJsonString.c_str());

    if (LoadDocument.HasParseError())
        return false;



    if (LoadDocument.HasMember("Assets") && LoadDocument["Assets"].IsArray())
    {
        for (const auto& AssetJson : LoadDocument["Assets"].GetArray())
        {
            FGuid AssetGuid;
            AssetGuid.Parse(AssetJson["Guid"].GetString());
            FString TypeName = AssetJson["TypeName"].GetString();

            FString AssetName = AssetJson["AssetName"].GetString();
            FString MetadataPath = AssetJson["MetadataPath"].GetString();

            auto EmptyAsset = TypeRegistry::Find(TypeName)->Creator();
            UObjectSystem::RegisterWithGuid(EmptyAsset.get(), AssetGuid);

            AssetRegistry->AdoptAsset(Device, AssetGuid, AssetName, MetadataPath, std::move(EmptyAsset));
        }
    }
    else
        return false;


    if (LoadDocument.HasMember("Actors") && LoadDocument["Actors"].IsArray())
    {
        // ==================================================================
        // 모든 AActor 껍데기 생성 및 GUID 등록, World 설정
        // 이후 모든 액터 내부의 "컴포넌트 껍데기" 생성 및 GUID 등록
        // ==================================================================
        size_t ActorIndex = 0;

        for (const auto& ActorJson : LoadDocument["Actors"].GetArray())
        {
            FGuid ActorGuid;
            ActorGuid.Parse(ActorJson["Guid"].GetString());
            FString TypeName = ActorJson["TypeName"].GetString();

            std::unique_ptr<UObject> CreatedObject = TypeRegistry::Find(TypeName)->Creator();
            Actors.push_back(std::unique_ptr<AActor>(static_cast<AActor*>(CreatedObject.release())));

            Actors.back()->SetWorld(this);
            UObjectSystem::RegisterWithGuid(Actors.back().get(), ActorGuid);

            FArchiveJson ArchiveLoad(const_cast<rapidjson::Value&>(ActorJson));
            Actors[ActorIndex]->PreLoadComponents(ArchiveLoad);
            ++ActorIndex;
        }


        // ==================================================================
        // 진짜 직렬화
        // ==================================================================
        ActorIndex = 0;
        for (auto& ActorJson : LoadDocument["Actors"].GetArray())
        {
            FArchiveJson ArchiveLoad(const_cast<rapidjson::Value&>(ActorJson));
            Actors[ActorIndex]->Load(ArchiveLoad); 
            ++ActorIndex;
        }
    }
    else
        return false;

    return true;
}