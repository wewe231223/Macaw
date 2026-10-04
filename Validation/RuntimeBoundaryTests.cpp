#include "World/UWorld.h"
#include "CoreUObject/TypeRegistry.h"
#include "CoreUObject/TObjectRef.h"
#include "Asset/FAssetRegistry.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>

class FRecordingObserver final : public IWorldObserver {
public:
    void OnWorldChanged(UWorld& World, EWorldChange Change, AActor* Actor) override;
    Uint32 GetAddedCount() const;
    Uint32 GetRemovedCount() const;
    Uint32 GetDestroyedCount() const;
    bool WasActorAliveWhenRemoved() const;

private:
    Uint32 mAddedCount{};
    Uint32 mRemovedCount{};
    Uint32 mDestroyedCount{};
    bool mActorAliveWhenRemoved{};
};

void FRecordingObserver::OnWorldChanged(UWorld&, EWorldChange Change, AActor* Actor) {
    if (Change == EWorldChange::ActorAdded) {
        ++mAddedCount;
    } else if (Change == EWorldChange::ActorRemoving) {
        ++mRemovedCount;
        mActorAliveWhenRemoved = Actor != nullptr && UObjectSystem::Resolve(Actor->GetHandle()) == Actor;
    } else if (Change == EWorldChange::Destroying) {
        ++mDestroyedCount;
    }
}

Uint32 FRecordingObserver::GetAddedCount() const {
    return mAddedCount;
}

Uint32 FRecordingObserver::GetRemovedCount() const {
    return mRemovedCount;
}

Uint32 FRecordingObserver::GetDestroyedCount() const {
    return mDestroyedCount;
}

bool FRecordingObserver::WasActorAliveWhenRemoved() const {
    return mActorAliveWhenRemoved;
}

static void Require(bool Condition, const char* Message) {
    if (!Condition) {
        throw std::runtime_error{Message};
    }
}

static void CheckWorldLifetime() {
    const Uint32 InitialCount{UObjectSystem::GetObjectCount()};
    FRecordingObserver Observer{};
    TObjectRef<AActor> ActorReference{};
    TObjectRef<USceneComponent> ComponentReference{};
    {
        UWorld World{};
        World.AddObserver(Observer);
        World.AddObserver(Observer);
        Require(World.GetRenderSubsystem().IsInitialized() && World.GetPickingSubsystem().IsInitialized(), "world services were not initialized");
        AActor* Actor{World.AdoptActor<AActor>()};
        USceneComponent* Root{Actor->AddComponent<USceneComponent>()};
        Require(Actor->SetRootComponent(Root), "root assignment failed");
        ActorReference.Set(Actor);
        ComponentReference.Set(Root);
        const Uint64 Revision{World.GetStructureRevision()};
        Require(World.RenameActor(Actor, FName{"BoundaryActor"}), "actor rename failed");
        Require(World.GetStructureRevision() > Revision && Observer.GetAddedCount() == 1, "world structure notification failed");
        FSceneRenderData Scene{};
        World.BuildSceneRenderData(Scene);
        Require(Actor->Destroy(), "actor destruction request failed");
        World.Tick(0.016f);
        Require(!ActorReference.IsValid() && !ComponentReference.IsValid(), "destroyed object handles remained valid");
        Require(Observer.GetRemovedCount() == 1 && Observer.WasActorAliveWhenRemoved(), "removal notification arrived after destruction");
    }
    Require(Observer.GetDestroyedCount() == 1, "world destruction notification failed");
    Require(UObjectSystem::GetObjectCount() == InitialCount, "world leaked registered objects");
}

static void CheckSceneRoundTrip() {
    TypeRegistry::Register(AActor::StaticTypeInfo());
    TypeRegistry::Register(USceneComponent::StaticTypeInfo());
    FAssetRegistry Registry{};
    const std::filesystem::path OriginalDirectory{std::filesystem::current_path()};
    const std::filesystem::path TemporaryDirectory{std::filesystem::temp_directory_path() / (std::string{"MacawBoundary-"} + FGuid::NewGuid().ToString().c_str())};
    std::filesystem::create_directories(TemporaryDirectory);
    std::filesystem::current_path(TemporaryDirectory);
    try {
        {
            UWorld World{};
            World.SetAssetRegistry(&Registry);
            AActor* Parent{World.AdoptActor<AActor>()};
            AActor* Child{World.AdoptActor<AActor>()};
            World.RenameActor(Parent, FName{"Parent"});
            World.RenameActor(Child, FName{"Child"});
            Parent->SetRootComponent(Parent->AddComponent<USceneComponent>());
            Child->SetRootComponent(Child->AddComponent<USceneComponent>());
            Parent->SetActorLocation(FVector3{3.0f, 4.0f, 5.0f});
            World.AttachActor(Child, Parent);
            Require(World.SaveScene("Boundary", &Registry), "scene save failed");
        }
        {
            UWorld Loaded{};
            Loaded.SetAssetRegistry(&Registry);
            Require(Loaded.LoadScene(TemporaryDirectory / "scenes/Boundary.json"), "scene load failed");
            AActor* Parent{Loaded.FindActorByName(FName{"Parent"})};
            AActor* Child{Loaded.FindActorByName(FName{"Child"})};
            Require(Parent != nullptr && Child != nullptr, "scene lost actors");
            Require(Child->GetRootComponent()->GetParent() == Parent->GetRootComponent(), "scene lost attachment references");
            Require(Parent->GetActorLocation().mX == 3.0f, "scene lost transforms");
        }
        std::filesystem::current_path(OriginalDirectory);
        std::filesystem::remove(TemporaryDirectory / "scenes/Boundary.json");
        std::filesystem::remove(TemporaryDirectory / "scenes");
        std::filesystem::remove(TemporaryDirectory);
    } catch (...) {
        std::filesystem::current_path(OriginalDirectory);
        throw;
    }
}

int main() {
    try {
        CheckWorldLifetime();
        CheckSceneRoundTrip();
        std::cout << "Runtime boundary tests passed without Editor, Render, or ImGui.\n";
        return 0;
    } catch (const std::exception& Error) {
        std::cerr << Error.what() << '\n';
        return 1;
    }
}
