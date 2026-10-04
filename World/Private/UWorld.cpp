#include "pch.h"
#include "World/UWorld.h"
#include "FTemporarySceneLoader.h"
#include "Core/Stat/Stat.h"

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <random>
#include "World/AActor.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UActorComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Subsystem/UCameraSubsystem.h"
#include "World/Subsystem/UCollisionSubsystem.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "World/Subsystem/UTextSubsystem.h"
#include "World/Subsystem/UBillboardSubsystem.h"
#include "World/Subsystem/ULightSubsystem.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/ULightComponent.h"
#include "Asset/Pipeline/UPipeline.h"
#include "Asset/UMesh.h"
#include "Asset/FAssetRegistry.h"
#include "Serialization/FArchiveJson.h"
#include "CoreUObject/TypeRegistry.h"
#include "CoreUObject/UObjectSystem.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Core/Console/Console.h"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include "World/Component/UNameTagComponent.h"

UWorld::UWorld() = default;

UWorld::~UWorld() {
    CleanupWorld();
}

void UWorld::Initialize(EWorldType WorldType) {
    if (mInitialized || mCleaningUp) {
        return;
    }
    mWorldType = WorldType;
    mTime.Reset();
    mPersistentLevel = std::make_unique<ULevel>(*this);
    UObjectSystem::Register(mPersistentLevel.get());
    mInitialized = true;
    InitializeSubsystems();
}

void UWorld::CleanupWorld() {
    if (!mInitialized || mCleaningUp) {
        return;
    }
    if (mBTickingActors || mBeginningPlay || mEndingPlay || mActorDispatchDepth > 0 || mFlushingActors) {
        throw std::logic_error{"Cannot clean up a world during actor callbacks"};
    }
    mCleaningUp = true;
    EndPlay();
    NotifyWorldChanged(EWorldChange::Destroying);
    mObservers.clear();
    ClearActors();
    DeinitializeSubsystems();
    UObjectSystem::Unregister(mPersistentLevel.get(), mPersistentLevel->GetHandle());
    mPersistentLevel.reset();
    mPendingDestroyActors.clear();
    mTickActors.clear();
    mTickActorsNeedCompaction = false;
    mEndPlayRequested = false;
    mAssetRegistry = nullptr;
    mAssetRegistryMutator = nullptr;
    mInitialized = false;
    mCleaningUp = false;
}

bool UWorld::IsInitialized() const {
    return mInitialized;
}

EWorldType UWorld::GetWorldType() const {
    return mWorldType;
}

ULevel& UWorld::GetPersistentLevel() {
    if (mPersistentLevel == nullptr) {
        throw std::logic_error{"World is not initialized"};
    }
    return *mPersistentLevel;
}

const ULevel& UWorld::GetPersistentLevel() const {
    if (mPersistentLevel == nullptr) {
        throw std::logic_error{"World is not initialized"};
    }
    return *mPersistentLevel;
}

void UWorld::BeginPlay() {
    if (!mInitialized || mHasBegunPlay || mBeginningPlay || mWorldType != EWorldType::Game || mCleaningUp || mLoadingScene || mEndingPlay) {
        return;
    }
    mTime.Reset();
    mBeginningPlay = true;
    try {
        for (std::size_t Index{}; Index < mPersistentLevel->mActors.size(); ++Index) {
            AActor* Actor{mPersistentLevel->mActors[Index].get()};
            if (std::ranges::find(mPendingDestroyActors, Actor) == mPendingDestroyActors.end()) {
                Actor->InitializeComponents();
            }
        }
        mHasBegunPlay = true;
        const std::size_t Count{mPersistentLevel->mActors.size()};
        for (std::size_t Index{}; Index < Count; ++Index) {
            AActor* Actor{mPersistentLevel->mActors[Index].get()};
            if (std::ranges::find(mPendingDestroyActors, Actor) == mPendingDestroyActors.end()) {
                Actor->DispatchBeginPlay();
            }
        }
    } catch (...) {
        mBeginningPlay = false;
        throw;
    }
    mBeginningPlay = false;
    if (mEndPlayRequested) {
        EndPlay();
    }
}

void UWorld::EndPlay() {
    if (!mHasBegunPlay || mEndingPlay) {
        return;
    }
    if (mBeginningPlay || mActorDispatchDepth > 0) {
        mEndPlayRequested = true;
        return;
    }
    mEndPlayRequested = false;
    mEndingPlay = true;
    mHasBegunPlay = false;
    for (std::size_t Index{mPersistentLevel->mActors.size()}; Index > 0; --Index) {
        mPersistentLevel->mActors[Index - 1]->DispatchEndPlay();
    }
    mEndingPlay = false;
}

bool UWorld::HasBegunPlay() const {
    return mHasBegunPlay;
}

TSubsystemCollection<UWorldSubsystem, UWorld>& UWorld::GetSubsystems() {
    return mSubsystems;
}

void UWorld::RefreshActorTicks() {
    for (const std::unique_ptr<AActor>& Actor : mPersistentLevel->mActors) {
        for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
            Actor->UpdateComponentTickRegistration(Component.get());
        }
        Actor->UpdateTickRegistration();
    }
}

AActor* UWorld::SpawnActor(const FAssetHandle& MeshHandle, const FAssetHandle& PipelineHandle, const FAssetHandle& MaterialHandle, const FVector3& Position) {
    if (mAssetRegistry == nullptr || mAssetRegistry->ResolveAsset<UMesh>(MeshHandle) == nullptr) {
        return nullptr;
    }

    std::unique_ptr<AActor> NewActor{std::make_unique<AActor>()};
    AActor* Actor{NewActor.get()};
    Actor->SetName(MakeUniqueObjectName(Actor->GetTypeInfo()->mTypeName));

    UStaticMeshComponent* MeshComponent{Actor->AddComponent<UStaticMeshComponent>()};
    if (MeshComponent == nullptr) {
        return nullptr;
    }
    Actor->SetRootComponent(MeshComponent);

    MeshComponent->SetMeshHandle(MeshHandle);
    MeshComponent->SetPipelineHandle(PipelineHandle);
    MeshComponent->SetMaterialHandle(MaterialHandle);

    MeshComponent->SetRelativeLocation(FVector3{ Position.mX, Position.mY, Position.mZ});

    UNameTagComponent* NameTagComponent{Actor->AddComponent<UNameTagComponent>()};
    NameTagComponent->AttachToComponent(MeshComponent);
    NameTagComponent->SetTargetActor(nullptr);
    NameTagComponent->SetTargetLocalOffset(NameTagComponent->GetTargetLocalOffset());
    NameTagComponent->SetVisible(true);
    NameTagComponent->SetActive(false);
    if (mAssetRegistry != nullptr) {
        NameTagComponent->SetPipelineHandle(mAssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Text.json"}));
        NameTagComponent->SetFontHandle(mAssetRegistry->FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"}));
    }

    return AddActor(std::move(NewActor));
}

bool UWorld::DestroyActor(AActor* Actor) {
    if (!mInitialized || Actor == nullptr) {
        return false;
    }

    auto It{std::ranges::find_if(mPersistentLevel->mActors, [Actor](const std::unique_ptr<AActor>& Ptr) {
        return Ptr.get() == Actor;
    })};

    if (It == mPersistentLevel->mActors.end()) {
        return false;
    }

    if (std::ranges::find(mPendingDestroyActors, Actor) != mPendingDestroyActors.end()) {
        return true;
    }

    mPendingDestroyActors.push_back(Actor);
    return true;
}

void UWorld::FlushPendingDestroyActors() {
    if (!mInitialized || mBTickingActors || mBeginningPlay || mEndingPlay || mFlushingActors || mActorDispatchDepth > 0) {
        return;
    }
    mFlushingActors = true;
    bool RemovedAnyActor{};
    while (!mPendingDestroyActors.empty()) {
        AActor* Actor{mPendingDestroyActors.back()};
        mPendingDestroyActors.pop_back();
        auto Iterator{std::ranges::find_if(mPersistentLevel->mActors, [Actor](const std::unique_ptr<AActor>& Candidate) {
            return Candidate.get() == Actor;
        })};
        if (Iterator == mPersistentLevel->mActors.end()) {
            continue;
        }
        std::unique_ptr<AActor> RemovedActor{std::move(*Iterator)};
        mPersistentLevel->mActors.erase(Iterator);
        NotifyWorldChanged(EWorldChange::ActorRemoving, Actor);
        Actor->SetWorld(nullptr);
        UObjectSystem::Unregister(Actor, Actor->GetHandle());
        RemovedAnyActor = true;
    }
    mFlushingActors = false;
    if (RemovedAnyActor) {
        MarkStructureDirty();
    }
}

void UWorld::AttachActor(AActor* Child, AActor* Parent) {
    if (Child == nullptr || Parent == nullptr || Child == Parent) {
        return;
    }

    USceneComponent* ChildRoot{Child->GetRootComponent()};
    USceneComponent* ParentRoot{Parent->GetRootComponent()};

    if (ChildRoot == nullptr || ParentRoot == nullptr) {
        return;
    }

    // 이미 같은 부모라면 변경 없음
    if (ChildRoot->GetParent() == ParentRoot) {
        return;
    }

    ChildRoot->AttachToComponent(ParentRoot);
    MarkStructureDirty();
}

void UWorld::DetachActor(AActor* Actor) {
    if (Actor == nullptr) {
        return;
    }

    USceneComponent* RootComponent{Actor->GetRootComponent()};

    if (RootComponent == nullptr) {
        return;
    }

    if (RootComponent->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform)) {
        MarkStructureDirty();
    }
}

bool UWorld::RenameActor(AActor* Actor, const FName& NewName) {
    if (Actor == nullptr) {
        return false;
    }

    if (Actor->GetWorld() != this) {
        return false;
    }

    if (Actor->GetName() == NewName) {
        return false;
    }

    Actor->SetName(NewName);

    MarkStructureDirty();

    return true;
}

const TArray<std::unique_ptr<AActor>>& UWorld::GetActors() const {
    return GetPersistentLevel().GetActors();
}

void UWorld::InitializeSubsystems() {
    mSubsystems.Initialize(*this);
    mSubsystems.Add<URenderSubsystem>();
    mSubsystems.Add<UCollisionSubsystem>();
    mSubsystems.Add<UPickingSubsystem>();
    mSubsystems.Add<UCameraSubsystem>();
    mSubsystems.Add<UTextSubsystem>();
    mSubsystems.Add<UBillboardSubsystem>();
    mSubsystems.Add<ULightSubsystem>();
}

void UWorld::DeinitializeSubsystems() {
    mSubsystems.Deinitialize();
}

UTextSubsystem& UWorld::GetTextSubsystem() {
    return *mSubsystems.Get<UTextSubsystem>();
}

const UTextSubsystem& UWorld::GetTextSubsystem() const {
    return *mSubsystems.Get<UTextSubsystem>();
}

ULightSubsystem& UWorld::GetLightSubsystem() {
    return *mSubsystems.Get<ULightSubsystem>();
}

const ULightSubsystem& UWorld::GetLightSubsystem() const {
    return *mSubsystems.Get<ULightSubsystem>();
}

void UWorld::BuildSceneRenderData(FSceneRenderData& Scene) {
    mSubsystems.Get<URenderSubsystem>()->BuildRenderProbes(Scene);

    mSubsystems.Get<ULightSubsystem>()->BuildLightProbes(Scene);
    mSubsystems.Get<UTextSubsystem>()->BuildTextProbes(Scene);
    mSubsystems.Get<UBillboardSubsystem>()->BuildRenderProbes(Scene);
}

void UWorld::MarkStructureDirty() {
    ++mStructureRevision;
    NotifyWorldChanged(EWorldChange::StructureChanged);
}

Uint64 UWorld::GetStructureRevision() const {
    return mStructureRevision;
}

void UWorld::Tick(float DeltaTime) {
    if (!mInitialized || mBTickingActors || mCleaningUp || mLoadingScene || mBeginningPlay || mEndingPlay || mActorDispatchDepth > 0) {
        return;
    }
    if (mWorldType == EWorldType::Game && !mHasBegunPlay) {
        FlushPendingDestroyActors();
        return;
    }
    mTime.Tick(static_cast<double>(DeltaTime));
    const float WorldDeltaTime{static_cast<float>(mTime.GetDeltaSeconds())};
    if (WorldDeltaTime > 0.0f && !mTickActors.empty()) {
        const Stat::FScopedWorldTickStatTimer TickStat{0};
        Stat::FWorldTickStats* TickStats{Stat::GetActiveWorldTickStats()};
        const bool WasTicking{mBTickingActors};
        mBTickingActors = true;
        const std::size_t ActorCount{mTickActors.size()};
        try {
            for (std::size_t Index{}; Index < ActorCount && Index < mTickActors.size(); ++Index) {
                AActor* Actor{mTickActors[Index]};
                if (Actor == nullptr || std::ranges::find(mPendingDestroyActors, Actor) != mPendingDestroyActors.end()) {
                    continue;
                }
                if (TickStats != nullptr) {
                    ++TickStats->mActorTickCount;
                }
                if (Actor->IsTickEnabled() && (Actor->HasBegunPlay() || Actor->IsTickInEditor())) {
                    Actor->Tick(WorldDeltaTime);
                } else {
                    Actor->AActor::Tick(WorldDeltaTime);
                }
            }
        } catch (...) {
            FinishActorTicks(WasTicking);
            throw;
        }
        FinishActorTicks(WasTicking);
    }

    FlushPendingDestroyActors();
}

void UWorld::RegisterTickActor(AActor* Actor) {
    if (Actor->mTickIndex != std::numeric_limits<std::size_t>::max()) {
        return;
    }

    mTickActors.push_back(Actor);
    Actor->mTickIndex = mTickActors.size() - 1;
}

void UWorld::UnregisterTickActor(AActor* Actor) {
    const std::size_t Index{Actor->mTickIndex};
    if (Index == std::numeric_limits<std::size_t>::max()) {
        return;
    }

    Actor->mTickIndex = std::numeric_limits<std::size_t>::max();
    if (mBTickingActors) {
        mTickActors[Index] = nullptr;
        mTickActorsNeedCompaction = true;
    } else {
        if (Index + 1 < mTickActors.size()) {
            AActor* LastActor{mTickActors.back()};
            mTickActors[Index] = LastActor;
            LastActor->mTickIndex = Index;
        }
        mTickActors.pop_back();
    }
}

void UWorld::FinishActorTicks(bool WasTicking) {
    mBTickingActors = WasTicking;
    if (WasTicking || !mTickActorsNeedCompaction) {
        return;
    }

    std::erase(mTickActors, nullptr);
    for (std::size_t Index{}; Index < mTickActors.size(); ++Index) {
        mTickActors[Index]->mTickIndex = Index;
    }
    mTickActorsNeedCompaction = false;
}

FWorldTime& UWorld::GetTime() {
    return mTime;
}

const FWorldTime& UWorld::GetTime() const {
    return mTime;
}

URenderSubsystem& UWorld::GetRenderSubsystem() {
    return *mSubsystems.Get<URenderSubsystem>();
}

const URenderSubsystem& UWorld::GetRenderSubsystem() const {
    return *mSubsystems.Get<URenderSubsystem>();
}

UCollisionSubsystem& UWorld::GetCollisionSubsystem() {
    return *mSubsystems.Get<UCollisionSubsystem>();
}

const UCollisionSubsystem& UWorld::GetCollisionSubsystem() const {
    return *mSubsystems.Get<UCollisionSubsystem>();
}

UPickingSubsystem& UWorld::GetPickingSubsystem() {
    return *mSubsystems.Get<UPickingSubsystem>();
}

const UPickingSubsystem& UWorld::GetPickingSubsystem() const {
    return *mSubsystems.Get<UPickingSubsystem>();
}

UCameraSubsystem& UWorld::GetCameraSubsystem() {
    return *mSubsystems.Get<UCameraSubsystem>();
}

const UCameraSubsystem& UWorld::GetCameraSubsystem() const {
    return *mSubsystems.Get<UCameraSubsystem>();
}

UBillboardSubsystem& UWorld::GetBillboardSubsystem() {
    return *mSubsystems.Get<UBillboardSubsystem>();
}

const UBillboardSubsystem& UWorld::GetBillboardSubsystem() const {
    return *mSubsystems.Get<UBillboardSubsystem>();
}

bool UWorld::SaveScene(const FString& SceneName, const IAssetRegistry* AssetRegistry) {
    if (!mInitialized) {
        return false;
    }
    std::filesystem::path CurrentPath{std::filesystem::current_path()};
    std::filesystem::path SceneDir{CurrentPath / "scenes"};
    if (!std::filesystem::exists(SceneDir))
        std::filesystem::create_directories(SceneDir);
    std::filesystem::path FilePath{SceneDir / (SceneName.c_str() + std::string(".json"))};

    rapidjson::Document Document{};
    Document.SetObject();
    rapidjson::Document::AllocatorType& Allocator{Document.GetAllocator()};

    FArchiveJson ArchiveSave{Document, Allocator};
    ArchiveSave.SetAssetResolver(AssetRegistry);

    Uint32 FormatVersion{2};
    ArchiveSave.Serialize("FormatVersion", FormatVersion);

    std::size_t ArraySize{static_cast<std::size_t>(mPersistentLevel->mActors.size())};
    ArchiveSave.BeginArrayScope("Actors", ArraySize);
    for (std::size_t CurrentIndex{0}, EndIndex{mPersistentLevel->mActors.size()}; CurrentIndex < EndIndex; ++CurrentIndex) {
        ArchiveSave.BeginObjectScope(std::to_string(CurrentIndex));
        mPersistentLevel->mActors[CurrentIndex]->Save(ArchiveSave);
        ArchiveSave.EndObjectScope();
    }
    ArchiveSave.EndArrayScope();

    std::ofstream OutputFileStream{FilePath};
    if (!OutputFileStream.is_open())
        return false;

    rapidjson::OStreamWrapper StreamWrapper{OutputFileStream};
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> Writer{StreamWrapper};
    Document.Accept(Writer);
    OutputFileStream.close();

    return true;
}

bool UWorld::LoadScene(const std::filesystem::path& ScenePath) {
    if (!mInitialized || mLoadingScene || mBTickingActors || mCleaningUp || mBeginningPlay || mEndingPlay || mActorDispatchDepth > 0 || mFlushingActors) {
        return false;
    }
    const bool WasPlaying{mHasBegunPlay};
    EndPlay();
    mLoadingScene = true;
    bool Loaded{};
    try {
        Loaded = LoadSceneInternal(ScenePath);
    } catch (...) {
        mLoadingScene = false;
        throw;
    }
    mLoadingScene = false;
    RefreshActorTicks();
    if (WasPlaying) {
        BeginPlay();
    }
    return Loaded;
}

bool UWorld::LoadSceneInternal(const std::filesystem::path& ScenePath) {
    const auto FinishLoad{[this]() {
        if (!GetPickingSubsystem().RebuildAccelerationStructure()) Console::AddLog(Console::STDOutHandle, ELogLevel::Warning, ELogCategory::Etc, "Failed to rebuild the picking acceleration structure after loading the scene.");
        MarkStructureDirty();
        return true;
    }};
    if (ScenePath.extension() == ".scene") {
        FAssetRegistry* Registry{dynamic_cast<FAssetRegistry*>(mAssetRegistryMutator)};
        if (Registry == nullptr) {
            return false;
        }

        FTemporarySceneLoader Loader{};
        const bool Loaded{Loader.Load(ScenePath, *this, *Registry)};
        if (!Loaded) {
            Console::AddLog(Console::STDOutHandle, ELogLevel::Error, ELogCategory::Etc, "Failed to load temporary scene: %s", ScenePath.generic_string().c_str());
        }
        return Loaded ? FinishLoad() : false;
    }

    std::ifstream InputFileStream{ScenePath};
    if (!InputFileStream.is_open()) {
        return false;
    }

    std::stringstream Buffer{};
    Buffer << InputFileStream.rdbuf();
    std::string LoadedJsonString{Buffer.str()};
    InputFileStream.close();

    rapidjson::Document LoadDocument{};
    LoadDocument.Parse(LoadedJsonString.c_str());

    if (LoadDocument.HasParseError() ||
        !LoadDocument.IsObject() ||
        !LoadDocument.HasMember("FormatVersion") ||
        !LoadDocument["FormatVersion"].IsUint() ||
        LoadDocument["FormatVersion"].GetUint() != 2 ||
        !LoadDocument.HasMember("Actors") ||
        !LoadDocument["Actors"].IsArray()) {
        return false;
    }

    if (mAssetRegistry == nullptr) {
        return false;
    }

    ClearActors();

    const auto FailLoad{[this]() {
        ClearActors();
        return false;
    }};

    for (rapidjson::Value& ActorJson : LoadDocument["Actors"].GetArray()) {
        if (!ActorJson.IsObject() ||
            !ActorJson.HasMember("Guid") || !ActorJson["Guid"].IsString() ||
            !ActorJson.HasMember("TypeName") || !ActorJson["TypeName"].IsString()) {
            return FailLoad();
        }

        FGuid ActorGuid{};
        if (!ActorGuid.Parse(ActorJson["Guid"].GetString())) {
            return FailLoad();
        }

        FString TypeName{ActorJson["TypeName"].GetString()};
        const FTypeInfo* Type{TypeRegistry::Find(TypeName)};
        if (Type == nullptr || Type->mCreator == nullptr) {
            return FailLoad();
        }

        std::unique_ptr<UObject> CreatedObject{Type->mCreator()};
        if (CreatedObject == nullptr ||
            !CreatedObject->GetTypeInfo()->IsA(AActor::StaticTypeInfo())) {
            return FailLoad();
        }

        std::unique_ptr<AActor> ActorPtr{static_cast<AActor*>(CreatedObject.release())};
        UObjectSystem::RegisterWithGuid(ActorPtr.get(), ActorGuid);

        FArchiveJson ArchiveLoad{ActorJson};
        if (!ActorPtr->PreLoadComponents(ArchiveLoad)) {
            UObjectSystem::Unregister(ActorPtr.get(), ActorPtr->GetHandle());
            return FailLoad();
        }

        mPersistentLevel->mActors.emplace_back(std::move(ActorPtr));
    }

    for (std::size_t ActorIndex{0}; ActorIndex < mPersistentLevel->mActors.size(); ++ActorIndex) {
        rapidjson::Value& ActorJson{LoadDocument["Actors"][static_cast<rapidjson::SizeType>(ActorIndex)]};
        FArchiveJson ArchiveLoad{ActorJson};
        ArchiveLoad.SetAssetResolver(mAssetRegistry);
        mPersistentLevel->mActors[ActorIndex]->Load(ArchiveLoad);
    }

    for (const std::unique_ptr<AActor>& Actor : mPersistentLevel->mActors) {
        if (!Actor->ResolveLoadedReferences()) {
            return FailLoad();
        }
    }

    ++mActorDispatchDepth;
    try {
        for (std::size_t Index{}; Index < mPersistentLevel->mActors.size(); ++Index) {
            mPersistentLevel->mActors[Index]->SetWorld(this);
        }
    } catch (...) {
        --mActorDispatchDepth;
        throw;
    }
    --mActorDispatchDepth;

    return FinishLoad();
}


AActor* UWorld::AddActor(std::unique_ptr<AActor> InActor) {
    if (!mInitialized || mCleaningUp || !InActor || InActor->GetWorld() != nullptr) {
        return nullptr;
    }

    AActor* Actor{InActor.get()};

    if (UObjectSystem::Resolve(Actor->GetHandle()) != Actor) {
        UObjectSystem::Register(Actor);
    }

    mPersistentLevel->mActors.push_back(std::move(InActor));

    ++mActorDispatchDepth;
    try {
        Actor->SetWorld(this);
        if (mHasBegunPlay && !mLoadingScene) {
            Actor->DispatchBeginPlay();
        }
        NotifyWorldChanged(EWorldChange::ActorAdded, Actor);
        MarkStructureDirty();
    } catch (...) {
        --mActorDispatchDepth;
        throw;
    }
    --mActorDispatchDepth;
    if (mEndPlayRequested) {
        EndPlay();
    }

    return Actor;
}

const IAssetRegistry* UWorld::GetAssetRegistry() const {
    return mAssetRegistry;
}

void UWorld::ClearActors() {
    for (auto& CurrentActor : mPersistentLevel->mActors) {
        DestroyActor(CurrentActor.get());
    }
    FlushPendingDestroyActors();
}

FName UWorld::MakeUniqueObjectName(std::string_view SourceName) {
    std::string_view BaseName{};
    Int32 Number{0};

    SplitNameAndNumber(SourceName, BaseName, Number);

    Int32 Index{(Number > 0) ? (Number + 1) : 1};

    if ((Number == 0) && (FindActorByName(BaseName) == nullptr)) {
        return FName{BaseName};
    }

    while (true) {
        FName CandidateName{BaseName, Index};

        if (FindActorByName(CandidateName) == nullptr) {
            return CandidateName;
        }

        Index++;
    }

    return FName{};
}

AActor* UWorld::FindActorByName(FName InName) const {
    if (!mInitialized) {
        return nullptr;
    }
    for (const auto& Actor : mPersistentLevel->mActors) {
        if (Actor && Actor->GetName() == InName) {
            return Actor.get();
        }
    }

    return nullptr;
}

void UWorld::SetAssetRegistry(const IAssetRegistry* InAssetRegistry, IAssetRegistryMutator* InAssetRegistryMutator) {
    mAssetRegistry = InAssetRegistry;
    mAssetRegistryMutator = InAssetRegistryMutator;
}

IAssetRegistryMutator* UWorld::GetAssetRegistryMutator() const {
    return mAssetRegistryMutator;
}

void UWorld::AddObserver(IWorldObserver& Observer) {
    if (std::ranges::find(mObservers, &Observer) == mObservers.end()) {
        mObservers.push_back(&Observer);
    }
}

void UWorld::RemoveObserver(IWorldObserver& Observer) {
    std::erase(mObservers, &Observer);
}

void UWorld::NotifyWorldChanged(EWorldChange Change, AActor* Actor) {
    const std::vector<IWorldObserver*> Observers{mObservers};
    for (IWorldObserver* Observer : Observers) {
        if (std::ranges::find(mObservers, Observer) != mObservers.end()) {
            Observer->OnWorldChanged(*this, Change, Actor);
        }
    }
}
