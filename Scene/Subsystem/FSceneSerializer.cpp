#include "PCH.h"
#include "FSceneSerializer.h"

#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <ranges>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include "Core/Asset/FAssetRegistry.h"
#include "Core/Base/TypeRegistry.h"
#include "Scene/AActor.h"
#include "Scene/Subsystem/FActorScene.h"
#include "Scene/Subsystem/FWorldContext.h"
#include "Serialize/FArchiveJson.h"

bool FSceneSerializer::Save(const IWorldSceneAccess &World, const FString &SceneName) const {
	FAssetRegistry *AssetRegistry = World.GetWorldContext().GetAssetRegistry();
	if (AssetRegistry == nullptr) {
		return false;
	}

	std::filesystem::path CurrentPath = std::filesystem::current_path();
	std::filesystem::path SceneDir = CurrentPath / "scenes";
	if (!std::filesystem::exists(SceneDir))
		std::filesystem::create_directories(SceneDir);
	std::filesystem::path FilePath = SceneDir / (SceneName.c_str() + std::string(".json"));

	rapidjson::Document Document;
	Document.SetObject();
	rapidjson::Document::AllocatorType &Allocator = Document.GetAllocator();

	FArchiveJson ArchiveSave(Document, Allocator);
	ArchiveSave.SetAssetRegistry(AssetRegistry);

	auto AssetList = AssetRegistry->GetAssetList();
	size_t ArraySize = static_cast<size_t>(AssetList.size());
	ArchiveSave.BeginArrayScope("Assets", ArraySize);

	for (size_t Index : std::views::iota(size_t{0}, std::ranges::size(AssetList))) {
		UObject *Asset = AssetList[Index];
		ArchiveSave.BeginObjectScope(std::to_string(Index));
		Asset->Save(ArchiveSave);
		ArchiveSave.EndObjectScope();
	}
	ArchiveSave.EndArrayScope();

	const TArray<std::unique_ptr<AActor>> &Actors = World.GetActorScene().GetActors();
	ArraySize = static_cast<size_t>(Actors.size());
	ArchiveSave.BeginArrayScope("Actors", ArraySize);
	for (size_t CurrentIndex = 0, EndIndex = Actors.size(); CurrentIndex < EndIndex; ++CurrentIndex) {
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

bool FSceneSerializer::Load(IWorldSceneAccess &World, const std::filesystem::path &ScenePath) const {
	FWorldContext &WorldContext = World.GetWorldContext();
	FAssetRegistry *AssetRegistry = WorldContext.GetAssetRegistry();
	ID3D11Device *Device = WorldContext.GetDevice();
	if (AssetRegistry == nullptr) {
		return false;
	}

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

	Reset(World);

	if (LoadDocument.HasMember("Assets") && LoadDocument["Assets"].IsArray()) {
		for (const auto &AssetJson : LoadDocument["Assets"].GetArray()) {
			FGuid AssetGuid;
			AssetGuid.Parse(AssetJson["Guid"].GetString());
			FString TypeName = AssetJson["TypeName"].GetString();
			FString AssetName = AssetJson["AssetName"].GetString();
			FString MetadataPath = AssetJson["AssetMetaDataPath"].GetString();

			auto EmptyAsset = TypeRegistry::Find(TypeName)->Creator();
			AssetRegistry->AdoptAsset(Device, AssetGuid, AssetName, MetadataPath, std::move(EmptyAsset));
		}
	} else {
		return false;
	}

	if (LoadDocument.HasMember("Actors") && LoadDocument["Actors"].IsArray()) {
		size_t ActorIndex = 0;
		for (auto &ActorJson : LoadDocument["Actors"].GetArray()) {
			FGuid ActorGuid;
			ActorGuid.Parse(ActorJson["Guid"].GetString());
			FString TypeName = ActorJson["TypeName"].GetString();

			std::unique_ptr<UObject> CreatedObject = TypeRegistry::Find(TypeName)->Creator();
			std::unique_ptr<AActor> ActorPtr(static_cast<AActor *>(CreatedObject.release()));
			FArchiveJson ArchiveLoad(static_cast<rapidjson::Value &>(ActorJson));
			World.GetActorScene().AddLoadedActor(std::move(ActorPtr), ActorGuid, ArchiveLoad);
			++ActorIndex;
		}

		ActorIndex = 0;
		for (auto &ActorJson : LoadDocument["Actors"].GetArray()) {
			FArchiveJson ArchiveLoad(static_cast<rapidjson::Value &>(ActorJson));
			ArchiveLoad.SetAssetRegistry(AssetRegistry);
			World.GetActorScene().GetActors()[ActorIndex]->Load(ArchiveLoad);
			++ActorIndex;
		}
	} else {
		return false;
	}

	return true;
}

void FSceneSerializer::Reset(IWorldSceneAccess &World) const {
	FWorldContext &WorldContext = World.GetWorldContext();
	FAssetRegistry *AssetRegistry = WorldContext.GetAssetRegistry();
	if (AssetRegistry == nullptr) {
		return;
	}

	World.GetActorScene().Reset();
	AssetRegistry->Reset();
	AssetRegistry->Initialize(WorldContext.GetDevice());
}
