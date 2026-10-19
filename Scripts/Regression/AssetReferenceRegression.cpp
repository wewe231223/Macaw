#include "AssetReferenceRegression.h"
#include "Core/Asset/FAssetReference.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "Engine/UEngine.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Asset/FAssetRegistry.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FArchiveMemory.h"
#include "World/UWorld.h"
#include "World/AActor.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "RenderCore/FPrimitiveSceneProxy.h"

#include <cstdio>
#include <filesystem>

FReferenceResolver::FReferenceResolver(FAssetHandle Handle, const FGuid& Guid, const FAssetPath& Path)
	: mHandle{Handle},
	  mGuid{Guid},
	  mPath{Path} {
}

FAssetHandle FReferenceResolver::FindAsset(const FAssetPath& Path) const {
    ++mPathLookupCount;
    return mAvailable && Path == mPath ? mHandle : FAssetHandle{};
}

FAssetHandle FReferenceResolver::FindAsset(const FGuid& Guid) const {
    return mAvailable && Guid == mGuid ? mHandle : FAssetHandle{};
}

const FAssetPath* FReferenceResolver::GetAssetPath(FAssetHandle Handle) const {
    return mAvailable && Handle == mHandle ? &mPath : nullptr;
}

const FGuid* FReferenceResolver::GetAssetGuid(FAssetHandle Handle) const {
    return mAvailable && Handle == mHandle ? &mGuid : nullptr;
}

void FReferenceResolver::SetAvailable(bool Available) {
    mAvailable = Available;
}

Uint32 FReferenceResolver::GetPathLookupCount() const {
    return mPathLookupCount;
}

namespace {
    bool Check(bool Passed, const char* Label) {
        std::printf("%s: %s\n", Passed ? "PASS" : "FAIL", Label);
        return Passed;
    }

    bool HasIdentity(const FAssetReference& Reference, const FGuid& Guid, const FAssetPath& Path) {
        return Reference.GetGuid() == Guid && Reference.GetPath() == Path;
    }

    bool RunValues() {
        const FAssetHandle Handle{7, 3};
        const FGuid Guid{1, 2, 3, 4};
        const FAssetPath Path{FString{"/Game/Test/Current"}};
        FReferenceResolver Resolver{Handle, Guid, Path};
        FAssetReference Reference{};
        bool Passed{Check(Reference.IsEmpty() && !Reference.Resolve(&Resolver), "Empty references stay empty")};

        Reference.Assign(Handle, &Resolver);
        Passed = Check(Reference.GetHandle() == Handle && HasIdentity(Reference, Guid, Path), "Assignment captures runtime and persistent identity") && Passed;
        Reference.Assign(Handle, nullptr);
        Passed = Check(HasIdentity(Reference, Guid, Path), "Reassigning the same handle without a resolver retains identity") && Passed;

        FAssetReference Copy{Reference};
        Reference.Assign({}, &Resolver);
        Passed = Check(Reference.IsEmpty() && Copy.GetHandle() == Handle && HasIdentity(Copy, Guid, Path), "Explicit clearing resets all identity and leaves value copies independent") && Passed;
        Copy.Assign(FAssetHandle{8, 3}, nullptr);
        Passed = Check(!Copy.GetGuid().IsValid() && !Copy.GetPath().IsValid(), "Replacing an asset never reuses the previous asset identity") && Passed;

        Reference = FAssetReference{Guid, FAssetPath{FString{"/Game/Test/Old"}}};
        const Uint32 PreviousPathLookups{Resolver.GetPathLookupCount()};
        Passed = Check(Reference.Resolve(&Resolver) == Handle && Resolver.GetPathLookupCount() == PreviousPathLookups && HasIdentity(Reference, Guid, Path), "GUID wins without a path lookup and refreshes a moved path") && Passed;

        Reference = FAssetReference{FGuid{9, 9, 9, 9}, Path};
        Passed = Check(Reference.Resolve(&Resolver) == Handle && HasIdentity(Reference, Guid, Path), "An unknown GUID falls back to the path and adopts the resolved identity") && Passed;
        Resolver.SetAvailable(false);
        Passed = Check(!Reference.Resolve(&Resolver) && !Reference.IsEmpty() && HasIdentity(Reference, Guid, Path), "A missing asset clears only the runtime handle") && Passed;
        Reference.Assign({}, &Resolver);
        Passed = Check(Reference.IsEmpty(), "Explicit clearing also works when the missing reference already has an empty handle") && Passed;
        Reference = FAssetReference{Guid, Path};
        Passed = Check(!Reference.Resolve(nullptr) && HasIdentity(Reference, Guid, Path), "Resolution without a resolver retains saved identity") && Passed;
        Resolver.SetAvailable(true);
        Passed = Check(Reference.Resolve(&Resolver) == Handle, "A previously missing reference can be resolved again") && Passed;
        return Passed;
    }

    bool RunArchives() {
        const FAssetHandle Handle{17, 5};
        const FGuid Guid{11, 12, 13, 14};
        const FAssetPath Path{FString{"/Game/Test/Archive"}};
        FReferenceResolver Resolver{Handle, Guid, Path};
        FAssetReference Source{};
        Source.Assign(Handle, &Resolver);
        TArray<Uint8> Bytes{};
        FArchiveMemory Save{Bytes};
        Save.SetAssetResolver(&Resolver);
        SerializeAssetReference(Save, "AssetGuid", "AssetPath", Source);

        TArray<Uint8> LegacyBytes{};
        FArchiveMemory LegacySave{LegacyBytes};
        FGuid LegacyGuid{Guid};
        FString LegacyPath{Path.mPath};
        LegacySave.Serialize("AssetGuid", LegacyGuid);
        LegacySave.Serialize("AssetPath", LegacyPath);
        bool Passed{Check(!Save.HasError() && Bytes == LegacyBytes, "Binary serialization preserves the existing GUID then path format")};

        FAssetReference Loaded{};
        FArchiveMemory Load{static_cast<const TArray<Uint8>&>(Bytes)};
        SerializeAssetReference(Load, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!Load.HasError() && !Loaded.GetHandle() && HasIdentity(Loaded, Guid, Path), "Loading without a resolver keeps persistent identity and no runtime handle") && Passed;

        TArray<Uint8> MissingBytes{};
        FArchiveMemory MissingSave{MissingBytes};
        Resolver.SetAvailable(false);
        MissingSave.SetAssetResolver(&Resolver);
        SerializeAssetReference(MissingSave, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!MissingSave.HasError() && MissingBytes == Bytes, "Saving a missing asset does not erase or replace its identity") && Passed;
        FArchiveMemory MissingLoad{static_cast<const TArray<Uint8>&>(Bytes)};
        MissingLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(MissingLoad, "AssetGuid", "AssetPath", Source);
        Passed = Check(!Source.GetHandle() && HasIdentity(Source, Guid, Path), "Loading with an unavailable resolver retains the missing asset") && Passed;
        Resolver.SetAvailable(true);
        Passed = Check(Loaded.Resolve(&Resolver) == Handle, "An archive loaded before the resolver is ready can resolve later") && Passed;

        rapidjson::Document Document{rapidjson::kObjectType};
        FArchiveJson JsonSave{Document, Document.GetAllocator()};
        JsonSave.SetAssetResolver(&Resolver);
        SerializeAssetReference(JsonSave, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(Document.HasMember("AssetGuid") && Document.HasMember("AssetPath") && !Document.HasMember("Handle"), "JSON retains legacy field names and omits runtime handles") && Passed;
        FAssetReference JsonLoaded{};
        FArchiveJson JsonLoad{Document};
        JsonLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(JsonLoad, "AssetGuid", "AssetPath", JsonLoaded);
        Passed = Check(!JsonLoad.HasError() && JsonLoaded.GetHandle() == Handle && HasIdentity(JsonLoaded, Guid, Path), "JSON round trip restores the resolved reference") && Passed;

        Document["AssetPath"].SetBool(false);
        FArchiveJson InvalidLoad{Document};
        SerializeAssetReference(InvalidLoad, "AssetGuid", "AssetPath", JsonLoaded);
        Passed = Check(InvalidLoad.HasError() && JsonLoaded.GetHandle() == Handle && HasIdentity(JsonLoaded, Guid, Path), "Malformed serialized identity leaves the reference unchanged") && Passed;
        JsonLoaded.Assign({}, &Resolver);
        rapidjson::Document Cleared{rapidjson::kObjectType};
        FArchiveJson ClearedSave{Cleared, Cleared.GetAllocator()};
        ClearedSave.SetAssetResolver(&Resolver);
        SerializeAssetReference(ClearedSave, "AssetGuid", "AssetPath", JsonLoaded);
        FArchiveJson ClearedLoad{Cleared};
        ClearedLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(ClearedLoad, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!ClearedLoad.HasError() && Loaded.IsEmpty(), "Explicitly cleared references remain empty after save and load") && Passed;

        rapidjson::Document Partial{rapidjson::kObjectType};
        Partial.AddMember("AssetPath", "/Game/Missing/PathOnly", Partial.GetAllocator());
        Loaded.Assign(Handle, &Resolver);
        FArchiveJson PathOnlyLoad{Partial};
        PathOnlyLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(PathOnlyLoad, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!PathOnlyLoad.HasError() && !Loaded.GetHandle() && !Loaded.GetGuid().IsValid() && Loaded.GetPath() == FAssetPath{FString{"/Game/Missing/PathOnly"}}, "Path-only JSON replaces the previous GUID instead of resolving the previous asset") && Passed;
        Partial["AssetPath"].SetString("");
        Loaded.Assign(Handle, &Resolver);
        FArchiveJson EmptyPathLoad{Partial};
        EmptyPathLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(EmptyPathLoad, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!EmptyPathLoad.HasError() && Loaded.IsEmpty(), "An explicitly empty path-only reference clears a previously assigned asset") && Passed;

        const FGuid MissingGuid{201, 202, 203, 204};
        rapidjson::Document GuidOnly{rapidjson::kObjectType};
        FArchiveJson GuidOnlySave{GuidOnly, GuidOnly.GetAllocator()};
        FGuid SavedGuid{MissingGuid};
        GuidOnlySave.Serialize("AssetGuid", SavedGuid);
        Loaded.Assign(Handle, &Resolver);
        FArchiveJson GuidOnlyLoad{GuidOnly};
        GuidOnlyLoad.SetAssetResolver(&Resolver);
        SerializeAssetReference(GuidOnlyLoad, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!GuidOnlyLoad.HasError() && !Loaded.GetHandle() && Loaded.GetGuid() == MissingGuid && !Loaded.GetPath().IsValid(), "GUID-only JSON never falls back to a path from the previous reference") && Passed;

        rapidjson::Document Absent{rapidjson::kObjectType};
        Loaded.Assign(Handle, nullptr);
        FArchiveJson AbsentLoad{Absent};
        SerializeAssetReference(AbsentLoad, "AssetGuid", "AssetPath", Loaded);
        Passed = Check(!AbsentLoad.HasError() && Loaded.GetHandle() == Handle, "Absent reference fields leave an existing runtime-only reference unchanged") && Passed;
        return Passed;
    }

    bool CheckProperty(UPrimitiveComponent& Component, const char* Name, const char* GuidName, const char* PathName, FAssetHandle Handle, UWorld& World) {
        const FProperty* Property{Component.GetClass()->FindProperty(FName{Name})};
        FAssetRegistry& Assets{FAssetRegistry::Get()};
        bool Passed{Check(Property != nullptr && Property->SetValue(Component, FPropertyValue{Handle}), Name)};

        if (!Passed) {
            return false;
        }

        rapidjson::Document Saved{rapidjson::kObjectType};
        FArchiveJson Save{Saved, Saved.GetAllocator()};
        Save.SetAssetResolver(&Assets);
        Property->SerializeContainer(&Component, Save);
        const FGuid MissingGuid{91, 92, 93, 94};
        const FAssetPath MissingPath{FString{"/Game/Missing/Asset"}};
        Passed = Check(!Save.HasError() && Saved[GuidName].GetString() == Assets.GetAssetGuid(Handle)->ToString() && Saved[PathName].GetString() == Assets.GetAssetPath(Handle)->mPath, "Component assignment serializes through the existing reflection hook") && Passed;

        FAssetReference Missing{MissingGuid, MissingPath};
        rapidjson::Document MissingDocument{rapidjson::kObjectType};
        FArchiveJson MissingSave{MissingDocument, MissingDocument.GetAllocator()};
        SerializeAssetReference(MissingSave, GuidName, PathName, Missing);
        FArchiveJson MissingLoad{MissingDocument};
        MissingLoad.SetAssetResolver(&Assets);
        Property->SerializeContainer(&Component, MissingLoad);
        Component.UnregisterComponent();
        Component.RegisterComponent(&World);

        rapidjson::Document AfterRegister{rapidjson::kObjectType};
        FArchiveJson AfterRegisterSave{AfterRegister, AfterRegister.GetAllocator()};
        AfterRegisterSave.SetAssetResolver(&Assets);
        Property->SerializeContainer(&Component, AfterRegisterSave);
        const FPropertyValue MissingValue{Property->GetValue(Component)};
        const FAssetHandle* MissingHandle{MissingValue.Get<FAssetHandle>()};
        Passed = Check(!MissingLoad.HasError() && !AfterRegisterSave.HasError() && MissingHandle != nullptr && !*MissingHandle && AfterRegister[GuidName].GetString() == MissingGuid.ToString() && AfterRegister[PathName].GetString() == MissingPath.mPath, "Component registration and default rendering retain a missing reference") && Passed;

        Passed = Check(Property->SetValue(Component, FPropertyValue{FAssetHandle{}}), "Reflection can explicitly clear a missing reference") && Passed;
        Component.UnregisterComponent();
        Component.RegisterComponent(&World);
        rapidjson::Document Cleared{rapidjson::kObjectType};
        FArchiveJson ClearedSave{Cleared, Cleared.GetAllocator()};
        ClearedSave.SetAssetResolver(&Assets);
        Property->SerializeContainer(&Component, ClearedSave);
        Passed = Check(!ClearedSave.HasError() && Cleared[GuidName].GetString() == FGuid{}.ToString() && Cleared[PathName].GetStringLength() == 0, "Explicit clearing stays empty across registration and saving") && Passed;

        FArchiveJson Restore{Saved};
        Restore.SetAssetResolver(&Assets);
        Property->SerializeContainer(&Component, Restore);
        const FPropertyValue Restored{Property->GetValue(Component)};
        const FAssetHandle* RestoredHandle{Restored.Get<FAssetHandle>()};
        Passed = Check(!Restore.HasError() && RestoredHandle != nullptr && *RestoredHandle == Handle, "Existing component JSON resolves back to the assigned handle") && Passed;
        return Passed;
    }

    bool HasSavedMaterial(UWorld& World, const FGuid& Guid, const FAssetPath& Path) {
        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                if (!Component->IsA(UStaticMeshComponent::StaticClass())) {
                    continue;
                }

                rapidjson::Document Document{rapidjson::kObjectType};
                FArchiveJson Save{Document, Document.GetAllocator()};
                Save.SetAssetResolver(FAssetRegistry::TryGet());
                Component->GetClass()->FindProperty(FName{"MaterialOverrides"})->SerializeContainer(Component.get(), Save);
                return !Save.HasError() && Document["MaterialOverrides"].Size() == 1 && Document["MaterialOverrides"][0]["MaterialGuid"].GetString() == Guid.ToString() && Document["MaterialOverrides"][0]["MaterialPath"].GetString() == Path.mPath;
            }
        }

        return false;
    }

    bool RunComponents() {
        FAssetRegistry& Assets{FAssetRegistry::Get()};
        bool Passed{Check(Assets.Initialize(), "Component asset fixtures initialize")};
        UWorld World{};
        World.Initialize();
        AActor* Actor{World.SpawnActor<AActor>()};
        UStaticMeshComponent* Mesh{Actor->AddComponent<UStaticMeshComponent>()};
        UBillboardComponent* Billboard{Actor->AddComponent<UBillboardComponent>()};
        UBillboardTextComponent* Text{Actor->AddComponent<UBillboardTextComponent>()};
        const FAssetHandle MeshHandle{Assets.FindAsset(FAssetPath{"/Game/System/Mesh/Cube.bin"})};
        const FAssetHandle MaterialHandle{Assets.EnsureDefaultStaticMeshMaterial()};
        const FAssetHandle TextureHandle{Assets.FindAsset(FAssetPath{"/Game/System/Light.png"})};
        const FAssetHandle FontHandle{Assets.FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"})};
        const FAssetHandle TextPipeline{Assets.FindAsset(FAssetPath{"/Game/Pipeline/Text.json"})};
        const FAssetHandle BillboardPipeline{Assets.FindAsset(FAssetPath{"/Game/Pipeline/Billboard.json"})};
        Passed = CheckProperty(*Mesh, "MeshHandle", "MeshAssetGuid", "MeshAssetPath", MeshHandle, World) && Passed;
        Passed = CheckProperty(*Billboard, "TextureHandle", "TextureAssetGuid", "TextureAssetPath", TextureHandle, World) && Passed;
        Passed = CheckProperty(*Billboard, "PipelineHandle", "PipelineAssetGuid", "PipelineAssetPath", BillboardPipeline, World) && Passed;
        Passed = CheckProperty(*Text, "FontHandle", "FontAssetGuid", "FontAssetPath", FontHandle, World) && Passed;
        Passed = CheckProperty(*Text, "PipelineHandle", "PipelineAssetGuid", "PipelineAssetPath", TextPipeline, World) && Passed;

        const FName Slot{Mesh->ResolveMesh()->GetMaterialSlotName(0)};
        Passed = Check(Mesh->SetMaterialOverride(Slot, MaterialHandle) && Mesh->GetMaterialOverride(Slot) == MaterialHandle, "A named override captures the selected material") && Passed;
        Text->SetText(FString{"Asset reference"});
        FBillboardDrawData BillboardData{};
        Passed = Check(Mesh->ResolveMesh() != nullptr && Mesh->CreateSceneProxy() != nullptr && Billboard->BuildBillboardDrawData(BillboardData) && BillboardData.mTextureHandle == TextureHandle && !Text->GetVertices().empty() && Text->CreateSceneProxy() != nullptr, "Resolved mesh, billboard and text still build their picking and rendering inputs") && Passed;
        Mesh->ClearMaterialOverrides();
        std::unique_ptr<FPrimitiveSceneProxy> Proxy{Mesh->CreateSceneProxy()};
        const FMeshSceneData* MeshData{Proxy != nullptr ? Proxy->GetMeshData() : nullptr};
        Passed = Check(MeshData != nullptr && MeshData->mMaterialOverrides.empty() && !Mesh->GetMaterialOverride(Slot) && Mesh->GetMaterial(0), "Mesh rendering uses mesh defaults without writing explicit overrides") && Passed;
        Text->SetFontHandle({});
        Text->SetPipelineHandle({});
        Passed = Check(!Text->GetFontHandle() && !Text->GetPipelineHandle() && !Text->GetVertices().empty() && Text->CreateSceneProxy() != nullptr, "Text default rendering remains available without changing cleared references") && Passed;

        const FGuid MissingGuid{101, 102, 103, 104};
        const FAssetPath MissingPath{FString{"/Game/Missing/Material"}};
        FAssetReference Missing{MissingGuid, MissingPath};
        rapidjson::Document MissingDocument{rapidjson::kObjectType};
        FArchiveJson MissingSave{MissingDocument, MissingDocument.GetAllocator()};
        TArray<FMeshMaterialOverride> MissingOverrides{FMeshMaterialOverride{Slot, Missing}};
        SerializeNativeValue(MissingSave, "MaterialOverrides", MissingOverrides);
        FArchiveJson MissingLoad{MissingDocument};
        MissingLoad.SetAssetResolver(&Assets);
        Mesh->GetClass()->FindProperty(FName{"MaterialOverrides"})->SerializeContainer(Mesh, MissingLoad);
        std::unique_ptr<UWorld> Clone{FWorldCloner::Clone(World)};
        Passed = Check(Clone != nullptr && HasSavedMaterial(*Clone, MissingGuid, MissingPath), "World memory cloning preserves missing identity across object creation and component registration") && Passed;
        Clone.reset();
        rapidjson::Document WorldDocument{rapidjson::kObjectType};
        FArchiveJson WorldSave{WorldDocument, WorldDocument.GetAllocator()};
        Passed = Check(FWorldSerializer::Save(World, WorldSave), "World JSON saves a missing asset reference") && Passed;
        World.CleanupWorld();
        World.Initialize();
        FArchiveJson WorldLoad{WorldDocument};
        Passed = Check(FWorldSerializer::Load(World, WorldLoad) && HasSavedMaterial(World, MissingGuid, MissingPath), "World JSON loading preserves missing identity through the complete lifecycle") && Passed;
        World.CleanupWorld();
        return Passed;
    }

    void ReregisterComponents(UWorld& World) {
        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                Component->UnregisterComponent();
                Component->RegisterComponent(&World);
            }
        }
    }

    bool HasSavedReferences(UWorld& World, const FGuid& Guid, const FAssetPath& Path) {
        Uint32 Count{};

        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                for (const FProperty* Property : Component->GetClass()->GetProperties()) {
                    if (!Property->HasAnyFlags(EPropertyFlags::Asset)) {
                        continue;
                    }

                    const FString Name{Property->GetName().ToString()};
                    const FString Prefix{Name.substr(0, Name.size() - 6)};
                    const FString GuidName{Prefix + "AssetGuid"};
                    const FString PathName{Prefix + "AssetPath"};
                    rapidjson::Document Document{rapidjson::kObjectType};
                    FArchiveJson Save{Document, Document.GetAllocator()};
                    Save.SetAssetResolver(FAssetRegistry::TryGet());
                    Property->SerializeContainer(Component.get(), Save);
                    FAssetReference Saved{};
                    FArchiveJson Load{Document};
                    SerializeAssetReference(Load, GuidName, PathName, Saved);
                    const FPropertyValue Value{Property->GetValue(*Component)};
                    const FAssetHandle* Handle{Value.Get<FAssetHandle>()};

                    if (Save.HasError() || Load.HasError() || Handle == nullptr || *Handle || !HasIdentity(Saved, Guid, Path)) {
                        return false;
                    }

                    ++Count;
                }
            }
        }

        return Count == 5;
    }

    bool RunLifecycleStates() {
        bool Passed{true};

        for (bool Missing : {true, false}) {
            UWorld World{};
            World.Initialize();
            AActor* Actor{World.SpawnActor<AActor>()};
            Actor->AddComponent<UStaticMeshComponent>();
            Actor->AddComponent<UBillboardComponent>();
            UBillboardTextComponent* Text{Actor->AddComponent<UBillboardTextComponent>()};
            Text->SetText(FString{"Reference lifecycle"});
            const FGuid MissingGuid{401, 402, 403, 404};
            const FAssetPath MissingPath{FString{"/Game/Missing/Lifecycle"}};
            const FGuid ExpectedGuid{Missing ? MissingGuid : FGuid{}};
            const FAssetPath ExpectedPath{Missing ? MissingPath : FAssetPath{}};
            bool Applied{true};

            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                for (const FProperty* Property : Component->GetClass()->GetProperties()) {
                    if (!Property->HasAnyFlags(EPropertyFlags::Asset)) {
                        continue;
                    }

                    const FString Name{Property->GetName().ToString()};
                    const FString Prefix{Name.substr(0, Name.size() - 6)};
                    FAssetReference Reference{MissingGuid, MissingPath};
                    rapidjson::Document Document{rapidjson::kObjectType};
                    FArchiveJson Save{Document, Document.GetAllocator()};
                    SerializeAssetReference(Save, Prefix + "AssetGuid", Prefix + "AssetPath", Reference);
                    FArchiveJson Load{Document};
                    Load.SetAssetResolver(FAssetRegistry::TryGet());
                    Property->SerializeContainer(Component.get(), Load);
                    Applied = !Load.HasError() && Applied;

                    if (!Missing) {
                        Applied = Property->SetValue(*Component, FPropertyValue{FAssetHandle{}}) && Applied;
                    }
                }
            }

            std::printf("Lifecycle state: %s\n", Missing ? "missing" : "explicitly cleared");
            Passed = Check(Applied && HasSavedReferences(World, ExpectedGuid, ExpectedPath), "all five properties distinguish missing identity from explicit clearing") && Passed;
            ReregisterComponents(World);
            Passed = Check(HasSavedReferences(World, ExpectedGuid, ExpectedPath), "Registration and rendering defaults preserve all five stored reference states") && Passed;
            std::unique_ptr<UWorld> Clone{FWorldCloner::Clone(World)};
            Passed = Check(Clone != nullptr && HasSavedReferences(*Clone, ExpectedGuid, ExpectedPath), "Memory cloning preserves every stored reference state") && Passed;

            if (Clone != nullptr) {
                ReregisterComponents(*Clone);
                Passed = Check(HasSavedReferences(*Clone, ExpectedGuid, ExpectedPath), "Reregistering the cloned components preserves missing and cleared references") && Passed;
            }

            Clone.reset();
            rapidjson::Document Document{rapidjson::kObjectType};
            FArchiveJson Save{Document, Document.GetAllocator()};
            Passed = Check(FWorldSerializer::Save(World, Save) && HasSavedReferences(World, ExpectedGuid, ExpectedPath), "World JSON saving preserves every reference without replacing it with a rendering default") && Passed;
            World.CleanupWorld();
            World.Initialize();
            FArchiveJson Load{Document};
            Passed = Check(FWorldSerializer::Load(World, Load) && HasSavedReferences(World, ExpectedGuid, ExpectedPath), "World JSON loading preserves every missing or cleared reference") && Passed;
            ReregisterComponents(World);
            Passed = Check(HasSavedReferences(World, ExpectedGuid, ExpectedPath), "Reregistration after JSON loading preserves every reference state") && Passed;
            Clone = FWorldCloner::Clone(World);
            Passed = Check(Clone != nullptr && HasSavedReferences(*Clone, ExpectedGuid, ExpectedPath), "Cloning after a JSON round trip preserves every reference state") && Passed;
            Clone.reset();
            World.CleanupWorld();
        }

        return Passed;
    }
}

int main() {
    bool Passed{RunValues()};
    Passed = RunArchives() && Passed;
    const std::filesystem::path Repository{std::filesystem::current_path()};
    const std::filesystem::path Sandbox{Repository / "Build/Diagnostics/AssetReferenceFixtures" / FGuid::NewGuid().ToString().c_str()};
    std::filesystem::create_directories(Sandbox / "Content");

    for (const char* Directory : {"System", "Shader", "Pipeline", "Font", "Texture"}) {
        std::filesystem::copy(Repository / "Content" / Directory, Sandbox / "Content" / Directory, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    }

    std::filesystem::current_path(Sandbox);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    UEngine Engine{};
    Engine.Initialize();
    Passed = RunComponents() && Passed;
    Passed = RunLifecycleStates() && Passed;
    Engine.Shutdown();
    CoUninitialize();
    std::filesystem::current_path(Repository);
    return Passed ? 0 : 1;
}
