#include "TransformHierarchyRegression.h"
#include "Engine/UEngine.h"
#include "Engine/Serialization/FWorldSerializer.h"
#include "Engine/Serialization/FWorldCloner.h"
#include "Engine/Serialization/FSceneSerializer.h"
#include "Editor/Property/FRotationEditorState.h"
#include "Serialization/FArchiveJson.h"
#include "Serialization/FArchiveMemory.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Subsystem/UPickingSubsystem.h"
#include "World/Subsystem/URenderSubsystem.h"
#include "Asset/FAssetRegistry.h"
#include "Render/FRenderScene.h"
#include "Render/FMaterialBuffer.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <type_traits>

UTransformProbe::UTransformProbe() {
    InitializeClass(StaticClass());
    SetPickingBox(DirectX::BoundingOrientedBox{DirectX::XMFLOAT3{}, DirectX::XMFLOAT3{0.5f, 0.5f, 0.5f}, DirectX::XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f}});
}

void UTransformProbe::OnTransformUpdate() {
    ++mNotificationCount;
    Super::OnTransformUpdate();

    if (mOnUpdate) {
        mOnUpdate();
    }
}

bool UTransformProbe::ShouldCreateRenderState() const {
    return true;
}

std::unique_ptr<FPrimitiveSceneProxy> UTransformProbe::CreateSceneProxy() const {
    return std::make_unique<FPrimitiveSceneProxy>(GetHandle(), GetOwner()->GetHandle(), GetRenderTransform());
}

Uint64 UTransformProbe::GetNotificationCount() const {
    return mNotificationCount;
}

void UTransformProbe::SetOnUpdate(std::function<void()> OnUpdate) {
    mOnUpdate = std::move(OnUpdate);
}

namespace {
    bool Check(bool Passed, const char* Label) {
        std::printf("%s: %s\n", Passed ? "PASS" : "FAIL", Label);
        return Passed;
    }

    bool Near(const FVector3& First, const FVector3& Second, float Tolerance = 0.001f) {
        return (First - Second).Length() <= Tolerance;
    }

    bool SameRotation(const FQuat& First, const FQuat& Second) {
        return Near(First.RotateVector(FVector3::UnitX), Second.RotateVector(FVector3::UnitX)) && Near(First.RotateVector(FVector3::UnitY), Second.RotateVector(FVector3::UnitY));
    }

    bool NearTransform(const FTransform& First, const FTransform& Second) {
        return Near(First.GetPosition(), Second.GetPosition()) && Near(First.GetScale(), Second.GetScale()) && SameRotation(First.GetRotationQuaternion(), Second.GetRotationQuaternion());
    }

    bool BoundsMatch(const UPrimitiveComponent& Component) {
        DirectX::BoundingOrientedBox Expected{};
        Component.GetPickingBox().Transform(Expected, Component.GetComponentToWorld().ToSimpleMath());

        return Near(FVector3{Expected.Center}, FVector3{Component.GetWorldOBB().Center}) && Near(FVector3{Expected.Extents}, FVector3{Component.GetWorldOBB().Extents}) && SameRotation(FQuat{Expected.Orientation}, FQuat{Component.GetWorldOBB().Orientation});
    }

    bool Hit(UWorld& World, UPrimitiveComponent& Component) {
        const FVector3 Position{Component.GetComponentLocation()};
        const FRay Ray{(Position + FVector3{0.0f, 0.0f, -10.0f}).ToSimpleMath(), FVector3::UnitZ.ToSimpleMath()};
        UPrimitiveComponent* Found{};
        float Distance{};

        return World.GetPickingSubsystem().Raycast(Ray, Found, Distance) && Found == &Component && std::isfinite(Distance);
    }

    bool SyncRender(UWorld& World, FRenderScene& Scene, const FMaterialBuffer& Materials, UPrimitiveComponent& Component) {
        FSceneUpdateBatch Updates{};
        World.GetRenderSubsystem().BuildSceneUpdates(Updates);
        Scene.Synchronize(Updates, Materials);
        const FPrimitiveSceneProxy* Proxy{Scene.FindPrimitive(Component.GetHandle())};

        return Proxy != nullptr && Proxy->GetTransform().mWorld == Component.GetComponentToWorld() && Near(FVector3{Proxy->GetTransform().mWorldOBB.Center}, FVector3{Component.GetWorldOBB().Center});
    }

    USceneComponent* Find(UWorld& World, const char* Name) {
        for (const std::unique_ptr<AActor>& Actor : World.GetActors()) {
            for (const std::unique_ptr<UActorComponent>& Component : Actor->GetComponents()) {
                if (Component->GetName() == FName{Name}) {
                    return static_cast<USceneComponent*>(Component.get());
                }
            }
        }

        return nullptr;
    }

    bool RunValues() {
        static_assert(std::is_trivially_copyable_v<FTransform>);
        const FTransform Relative{FVector3{1.0f, 2.0f, 3.0f}, FRotator{10.0f, 30.0f, 5.0f}, FVector3{0.5f, 2.0f, -1.0f}};
        const FTransform Parent{FVector3{10.0f, 0.0f, 0.0f}, FRotator{0.0f, 90.0f, 0.0f}, FVector3{2.0f, 3.0f, 4.0f}};
        const FTransform World{Relative.Compose(Parent)};
        FTransform Restored{};
        bool Passed{Check(Near(World.GetPosition(), FVector3{4.0f, 2.0f, 12.0f}) && World.MakeRelativeTo(Parent, Restored) && NearTransform(Relative, Restored), "TRS composition and relative inversion support rotation, nonuniform and negative scale")};

        FTransform Copy{Relative};
        FTransform Assigned{};
        Assigned = Copy;
        Copy.SetPosition(FVector3{50.0f, 60.0f, 70.0f});
        Passed = Check(Assigned == Relative && !(Copy == Relative) && Assigned.ToMatrixWithScale() == Relative.ToMatrixWithScale(), "Transform copy and assignment are independent value copies") && Passed;

        FTransform Singular{Parent};
        Singular.SetScale(FVector3{0.0f, 1.0f, 1.0f});
        const FTransform Before{Restored};
        Passed = Check(!World.MakeRelativeTo(Singular, Restored) && Restored == Before, "A singular relative conversion leaves its output unchanged") && Passed;

        FTransform Alias{World};
        Passed = Check(Alias.MakeRelativeTo(Parent, Alias) && NearTransform(Alias, Relative), "Relative conversion supports an aliased output value") && Passed;
        return Passed;
    }

    bool RunHierarchy() {
        UWorld World{};
        World.Initialize();
        AActor* Actor{World.SpawnActorDeferred<AActor>()};
        UTransformProbe* Root{Actor->AddComponent<UTransformProbe>()};
        UTransformProbe* Child{Actor->AddComponent<UTransformProbe>()};
        UTransformProbe* Leaf{Actor->AddComponent<UTransformProbe>()};
        USceneComponent* Other{Actor->AddComponent<USceneComponent>()};
        Actor->SetRootComponent(Root);
        Child->AttachToComponent(Root);
        Leaf->AttachToComponent(Child);
        Child->SetRelativeLocation(FVector3{2.0f, 0.0f, 0.0f});
        Leaf->SetRelativeLocation(FVector3{0.0f, 3.0f, 0.0f});
        World.FinishSpawningActor(Actor, FTransform{});

        World.GetPickingSubsystem().RebuildAccelerationStructure();
        FRenderScene Scene{123};
        FMaterialBuffer Materials{};
        bool Passed{Check(SyncRender(World, Scene, Materials, *Leaf) && Hit(World, *Leaf), "Initial scene proxies and Picking consume the hierarchy")};
        bool CallbackSawUpdatedSubtree{};
        Root->SetOnUpdate([&] {
            CallbackSawUpdatedSubtree = Near(Leaf->GetComponentLocation(), FVector3{12.0f, 3.0f, 0.0f}) && BoundsMatch(*Leaf);
        });
        const Uint64 RootCount{Root->GetNotificationCount()};
        const Uint64 ChildCount{Child->GetNotificationCount()};
        const Uint64 LeafCount{Leaf->GetNotificationCount()};
        Root->SetRelativeLocation(FVector3{10.0f, 0.0f, 0.0f});
        Passed = Check(CallbackSawUpdatedSubtree && Root->GetNotificationCount() == RootCount + 1 && Child->GetNotificationCount() == ChildCount + 1 && Leaf->GetNotificationCount() == LeafCount + 1, "Mutation calculates the entire subtree before issuing one notification per component") && Passed;
        Root->SetOnUpdate({});

        const Uint64 Revision{Leaf->GetTransformRevision()};
        const Uint64 Notifications{Leaf->GetNotificationCount()};

        for (Uint32 Index{}; Index < 4; ++Index) {
            Leaf->GetWorldAABB();
            Root->GetComponentTransform();
            Leaf->GetComponentToWorld();
            Leaf->GetWorldSphere();
            Leaf->GetWorldOBB();
            Leaf->GetComponentTransform();
            Leaf->GetTransformRevision();
        }

        Root->SetRelativeTransform(Root->GetRelativeTransform());
        Passed = Check(Leaf->GetTransformRevision() == Revision && Leaf->GetNotificationCount() == Notifications && Hit(World, *Leaf) && SyncRender(World, Scene, Materials, *Leaf), "Query order and unchanged assignments do not alter notifications, revisions, Picking or Render") && Passed;
        Root->SetRelativeRotation(FRotator{0.0f, 90.0f, 0.0f});
        Passed = Check(Near(Leaf->GetComponentLocation(), FVector3{7.0f, 2.0f, 0.0f}) && BoundsMatch(*Leaf) && Hit(World, *Leaf) && SyncRender(World, Scene, Materials, *Leaf), "Parent rotation updates child transforms, Bounds, Picking and scene proxy transforms") && Passed;

        Other->SetRelativeLocation(FVector3{-5.0f, 7.0f, 0.0f});
        const FTransform BeforeAttach{Leaf->GetComponentTransform()};
        Passed = Check(Child->AttachToComponent(Other, EAttachmentTransformRule::KeepWorldTransform) && NearTransform(Leaf->GetComponentTransform(), BeforeAttach) && BoundsMatch(*Leaf), "KeepWorld attachment preserves descendant transforms and Bounds") && Passed;
        Other->SetRelativeLocation(FVector3{4.0f, 5.0f, 0.0f});
        Passed = Check(Near(Leaf->GetComponentLocation(), BeforeAttach.GetPosition() + FVector3{9.0f, -2.0f, 0.0f}) && Hit(World, *Leaf) && SyncRender(World, Scene, Materials, *Leaf), "Reparented descendants follow only their new parent") && Passed;
        const FTransform BeforeDetach{Leaf->GetComponentTransform()};
        Passed = Check(Child->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform) && NearTransform(Leaf->GetComponentTransform(), BeforeDetach) && BoundsMatch(*Leaf), "KeepWorld detachment updates the same transform and Bounds path") && Passed;
        Other->SetRelativeLocation(FVector3{100.0f, 0.0f, 0.0f});
        Passed = Check(NearTransform(Leaf->GetComponentTransform(), BeforeDetach), "A detached subtree no longer follows its former parent") && Passed;

        Child->AttachToComponent(Other, EAttachmentTransformRule::KeepRelativeTransform);
        const FTransform BeforeDestroy{Leaf->GetComponentTransform()};
        Other->DestroyComponent();
        Passed = Check(Child->GetParent() == nullptr && NearTransform(Leaf->GetComponentTransform(), BeforeDestroy) && BoundsMatch(*Leaf), "Parent destruction detaches surviving children without stale world data") && Passed;

        const FTransform RelativeBeforeAttach{Child->GetRelativeTransform()};
        Passed = Check(Child->AttachToComponent(Root) && Child->GetRelativeTransform() == RelativeBeforeAttach && BoundsMatch(*Leaf) && !Root->AttachToComponent(Leaf), "KeepRelative attachment preserves local values and rejects cycles") && Passed;
        Root->SetRelativeScale3D(FVector3{0.0f, 2.0f, 3.0f});
        USceneComponent* Middle{Actor->AddComponent<USceneComponent>()};
        UTransformProbe* Survivor{Actor->AddComponent<UTransformProbe>()};
        Middle->AttachToComponent(Root);
        Survivor->AttachToComponent(Middle);
        Survivor->SetRelativeLocation(FVector3{3.0f, 4.0f, 5.0f});
        const FTransform BeforeSingularDestroy{Survivor->GetComponentTransform()};
        Middle->DestroyComponent();
        Passed = Check(Survivor->GetParent() == nullptr && NearTransform(Survivor->GetRelativeTransform(), BeforeSingularDestroy) && NearTransform(Survivor->GetComponentTransform(), BeforeSingularDestroy) && BoundsMatch(*Survivor), "Destroying a component below a singular ancestor detaches survivors with their world values") && Passed;
        World.CleanupWorld();
        return Passed;
    }

    bool RunAbsoluteOptions() {
        UWorld World{};
        World.Initialize();
        AActor* Actor{World.SpawnActorDeferred<AActor>()};
        USceneComponent* Parent{Actor->AddComponent<USceneComponent>()};
        UTransformProbe* Child{Actor->AddComponent<UTransformProbe>()};
        USceneComponent* Other{Actor->AddComponent<USceneComponent>()};
        Actor->SetRootComponent(Parent);
        Child->AttachToComponent(Parent);
        const FTransform ParentTransform{FVector3{10.0f, 20.0f, 30.0f}, FRotator{0.0f, 90.0f, 0.0f}, FVector3{2.0f, 3.0f, 4.0f}};
        const FTransform Relative{FVector3{1.0f, 2.0f, 3.0f}, FRotator{20.0f, 35.0f, 5.0f}, FVector3{0.5f, 2.0f, -1.0f}};
        const FTransform Desired{FVector3{-20.0f, 12.0f, 8.0f}, FRotator{30.0f, -25.0f, 15.0f}, FVector3{2.0f, -1.0f, 3.0f}};
        World.FinishSpawningActor(Actor, ParentTransform);
        Other->SetRelativeTransform(FTransform{FVector3{3.0f, -4.0f, 5.0f}, FRotator{12.0f, 15.0f, -30.0f}, FVector3{2.0f, 1.0f, -2.0f}});
        bool Passed{true};

        for (Uint32 Mask{}; Mask < 8; ++Mask) {
            const bool Location{(Mask & 1) != 0};
            const bool Rotation{(Mask & 2) != 0};
            const bool Scale{(Mask & 4) != 0};
            Child->AttachToComponent(Parent);
            Child->SetAbsolute(Location, Rotation, Scale);
            Child->SetRelativeTransform(Relative);
            const FVector3 ExpectedPosition{Location ? Relative.GetPosition() : FVector3{4.0f, 22.0f, 42.0f}};
            const FVector3 ExpectedScale{Scale ? Relative.GetScale() : FVector3{1.0f, 6.0f, -4.0f}};
            const FQuat ExpectedRotation{Rotation ? Relative.GetRotationQuaternion() : FQuat::Concatenate(ParentTransform.GetRotationQuaternion(), Relative.GetRotationQuaternion())};
            Passed = Check(Near(Child->GetComponentLocation(), ExpectedPosition) && Near(Child->GetComponentScale(), ExpectedScale) && SameRotation(Child->GetComponentTransform().GetRotationQuaternion(), ExpectedRotation) && BoundsMatch(*Child), "Absolute option combination applies independently to location, rotation and scale") && Passed;
            Parent->SetRelativeLocation(ParentTransform.GetPosition() + FVector3{7.0f, -3.0f, 2.0f});
            Passed = Check(Near(Child->GetComponentLocation(), Location ? ExpectedPosition : ExpectedPosition + FVector3{7.0f, -3.0f, 2.0f}) && Near(Child->GetComponentScale(), ExpectedScale) && SameRotation(Child->GetComponentTransform().GetRotationQuaternion(), ExpectedRotation) && BoundsMatch(*Child), "Parent movement preserves every absolute option combination") && Passed;
            Parent->SetRelativeTransform(ParentTransform);
            Passed = Check(Child->SetWorldTransform(Desired) && NearTransform(Child->GetComponentTransform(), Desired) && Child->IsAbsoluteLocation() == Location && Child->IsAbsoluteRotation() == Rotation && Child->IsAbsoluteScale() == Scale, "World assignment respects component absolute options") && Passed;
            Passed = Check(Child->AttachToComponent(Other, EAttachmentTransformRule::KeepWorldTransform) && NearTransform(Child->GetComponentTransform(), Desired) && Child->DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform) && NearTransform(Child->GetComponentTransform(), Desired) && BoundsMatch(*Child), "KeepWorld Attach and Detach respect every absolute option combination") && Passed;
        }

        Parent->SetRelativeScale3D(FVector3{0.0f, 2.0f, 3.0f});
        Child->SetAbsolute(false, false, false);
        const FTransform Before{Child->GetRelativeTransform()};
        const Uint64 BeforeRevision{Child->GetTransformRevision()};
        Passed = Check(!Child->AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform) && Child->GetParent() == nullptr && Child->GetRelativeTransform() == Before && Child->GetTransformRevision() == BeforeRevision, "A singular KeepWorld attachment fails without changing the graph or values") && Passed;
        Child->SetAbsolute(true, false, true);
        Passed = Check(Child->AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform) && Child->SetWorldTransform(Desired) && NearTransform(Child->GetComponentTransform(), Desired), "Absolute location and scale do not require an invertible parent scale") && Passed;
        Child->SetAbsolute(false, false, false);
        const FTransform BeforeFailure{Child->GetRelativeTransform()};
        Passed = Check(!Child->SetWorldTransform(Desired) && Child->GetRelativeTransform() == BeforeFailure, "A singular world assignment leaves relative state unchanged") && Passed;

        const FProperty* Property{Child->GetClass()->FindProperty(FName{"bAbsoluteLocation"})};
        Passed = Check(Property != nullptr && Property->SetValue(*Child, FPropertyValue{true}) && Child->IsAbsoluteLocation() && Near(Child->GetComponentLocation(), Child->GetRelativeLocation()) && BoundsMatch(*Child), "Reflected absolute option edits use the component update path") && Passed;
        World.CleanupWorld();
        return Passed;
    }

    bool RunArchives() {
        UWorld World{};
        World.Initialize();
        AActor* Actor{World.SpawnActorDeferred<AActor>()};
        UTransformProbe* Leaf{Actor->AddComponent<UTransformProbe>()};
        USceneComponent* Root{Actor->AddComponent<USceneComponent>()};
        UTransformProbe* Child{Actor->AddComponent<UTransformProbe>()};
        Root->SetName(FName{"ArchiveRoot"});
        Child->SetName(FName{"ArchiveChild"});
        Leaf->SetName(FName{"ArchiveLeaf"});
        Actor->SetRootComponent(Root);
        Leaf->AttachToComponent(Child);
        Child->AttachToComponent(Root);
        World.FinishSpawningActor(Actor, FTransform{FVector3{10.0f, 20.0f, 30.0f}, FRotator{0.0f, 90.0f, 0.0f}, FVector3{2.0f, 3.0f, 4.0f}});

        const FProperty* Property{Child->GetClass()->FindProperty(FName{"Transform"})};
        TArray<Uint8> LegacyBytes{};
        FArchiveMemory LegacySave{LegacyBytes};
        FVector3 Position{1.0f, 2.0f, 3.0f};
        FQuat Rotation{};
        FVector3 Scale{2.0f, 2.0f, 2.0f};
        bool LocationAbsolute{true};
        bool RotationAbsolute{false};
        bool ScaleAbsolute{true};
        LegacySave.Serialize("Position", Position);
        LegacySave.Serialize("Rotation", Rotation);
        LegacySave.Serialize("Scale", Scale);
        LegacySave.Serialize("bAbsoluteLocation", LocationAbsolute);
        LegacySave.Serialize("bAbsoluteRotation", RotationAbsolute);
        LegacySave.Serialize("bAbsoluteScale", ScaleAbsolute);
        const TArray<Uint8>& LegacyInput{LegacyBytes};
        FArchiveMemory LegacyLoad{LegacyInput};
        Property->SerializeContainer(Child, LegacyLoad);
        TArray<Uint8> SavedBytes{};
        FArchiveMemory CurrentSave{SavedBytes};
        Property->SerializeContainer(Child, CurrentSave);
        bool Passed{Check(!LegacyLoad.HasError() && LegacyLoad.GetRemainingBytes() == 0 && Child->IsAbsoluteLocation() && !Child->IsAbsoluteRotation() && Child->IsAbsoluteScale() && Near(Child->GetComponentLocation(), Position) && SavedBytes == LegacyBytes, "Legacy component Transform bytes retain field order while options are owned by the component")};

        rapidjson::Document LegacyJson{};
        LegacyJson.Parse(R"({"Transform":{"Position":[1,2,3],"Rotation":[0,0,0,1],"Scale":[2,2,2],"bAbsoluteLocation":true,"bAbsoluteRotation":false,"bAbsoluteScale":true}})");
        FArchiveJson LegacyJsonLoad{LegacyJson};
        Property->SerializeContainer(Child, LegacyJsonLoad);
        rapidjson::Document SavedJson{};
        SavedJson.SetObject();
        FArchiveJson LegacyJsonSave{SavedJson, SavedJson.GetAllocator()};
        Property->SerializeContainer(Child, LegacyJsonSave);
        Passed = Check(!LegacyJsonLoad.HasError() && SavedJson == LegacyJson && BoundsMatch(*Child), "Existing nested JSON options round trip without a schema migration") && Passed;

        Leaf->SetRelativeTransform(FTransform{FVector3{4.0f, 5.0f, 6.0f}, FRotator{95.0f, 720.0f, -370.0f}, FVector3{1.0f, 2.0f, -3.0f}});
        Leaf->SetAbsolute(false, true, false);
        const FTransform Expected{Leaf->GetComponentTransform()};
        rapidjson::Document Document{};
        Document.SetObject();
        FArchiveJson Save{Document, Document.GetAllocator()};
        Passed = Check(FWorldSerializer::Save(World, Save), "World JSON saves component-owned options") && Passed;

        std::unique_ptr<UWorld> Clone{FWorldCloner::Clone(World)};
        USceneComponent* CloneLeaf{Clone != nullptr ? Find(*Clone, "ArchiveLeaf") : nullptr};
        Passed = Check(CloneLeaf != nullptr && CloneLeaf->IsAbsoluteRotation() && NearTransform(CloneLeaf->GetComponentTransform(), Expected) && BoundsMatch(*static_cast<UPrimitiveComponent*>(CloneLeaf)), "Memory cloning restores options and a hierarchy whose component order differs from parent order") && Passed;

        World.CleanupWorld();
        World.Initialize();
        FArchiveJson Load{Document};
        Passed = Check(FWorldSerializer::Load(World, Load), "World JSON loads the unchanged Transform schema") && Passed;
        USceneComponent* LoadedLeaf{Find(World, "ArchiveLeaf")};
        USceneComponent* LoadedChild{Find(World, "ArchiveChild")};
        Passed = Check(LoadedLeaf != nullptr && LoadedChild != nullptr && LoadedLeaf->GetParent() == LoadedChild && LoadedLeaf->IsAbsoluteRotation() && LoadedChild->IsAbsoluteLocation() && LoadedChild->IsAbsoluteScale() && NearTransform(LoadedLeaf->GetComponentTransform(), Expected) && BoundsMatch(*static_cast<UPrimitiveComponent*>(LoadedLeaf)), "Loaded component options, parent references, world transforms and Bounds agree") && Passed;
        World.CleanupWorld();

        UWorld Sample{};
        Sample.Initialize();
        Passed = Check(FSceneSerializer::Load(Sample, "scenes/Default.json"), "The existing sample scene loads through the component Transform serializer") && Passed;
        Sample.CleanupWorld();
        return Passed;
    }

    bool RunRotationEditing() {
        FRotationEditorState State{};
        bool Passed{true};

        for (float Pitch : {-90.0f, 90.0f}) {
            FRotationEditorState Initial{};
            const FQuat Singular{FQuat::FromRotator(FRotator{Pitch, 35.0f, 25.0f})};
            Passed = Check(SameRotation(Singular, FQuat::FromRotator(Initial.Update(Singular))), "Initial Euler display preserves a singular Quaternion rotation") && Passed;
        }

        for (float Pitch : {-90.001f, -89.999f, -89.99f, 89.99f, 89.999f, 90.001f}) {
            FRotationEditorState Initial{};
            const FQuat Input{FQuat::FromRotator(FRotator{Pitch, 35.0f, 25.0f})};
            const FQuat Displayed{FQuat::FromRotator(Initial.Update(Input))};
            Passed = Check(Near(Input.RotateVector(FVector3::UnitX), Displayed.RotateVector(FVector3::UnitX), 0.00001f) && Near(Input.RotateVector(FVector3::UnitY), Displayed.RotateVector(FVector3::UnitY), 0.00001f), "Quaternion to editor Euler conversion preserves rotations near a singularity") && Passed;
        }

        bool KeptEdits{true};

        for (float Pitch : {89.0f, 90.0f, 91.0f, 95.0f, 180.0f, 270.0f, 450.0f}) {
            const FRotator Entered{Pitch, 725.0f, -370.0f};
            State.SetRotation(Entered);
            FQuat Rotation{FQuat::FromRotator(Entered)};
            Rotation.Normalize();
            const FRotator Actual{State.Update(Rotation)};
            KeptEdits = Actual.Pitch == Entered.Pitch && Actual.Yaw == Entered.Yaw && Actual.Roll == Entered.Roll && KeptEdits;
            const FQuat Opposite{-Rotation.X, -Rotation.Y, -Rotation.Z, -Rotation.W};
            KeptEdits = State.Update(Opposite).Pitch == Entered.Pitch && KeptEdits;
        }

        Passed = Check(KeptEdits, "Euler edits retain winding and gimbal-adjacent values after normalized Quaternion round trips") && Passed;
        State.SetRotation(FRotator{89.0f, 30.0f, 20.0f});
        const FRotator Crossing{State.Update(FQuat::FromRotator(FRotator{91.0f, 30.0f, 20.0f}))};
        Passed = Check(std::abs(Crossing.Pitch - 91.0f) < 0.01f && std::abs(Crossing.Yaw - 30.0f) < 0.01f && std::abs(Crossing.Roll - 20.0f) < 0.01f, "External rotation changes choose the closest equivalent Euler representation") && Passed;
        State.SetRotation(FRotator{0.0f, 179.0f, 0.0f});
        Passed = Check(std::abs(State.Update(FQuat::FromRotator(FRotator{0.0f, -179.0f, 0.0f})).Yaw - 181.0f) < 0.01f, "External rotation changes cross the 180 degree boundary continuously") && Passed;
        return Passed;
    }

    bool RunMeshPicking() {
        FAssetRegistry& Assets{FAssetRegistry::Get()};
        bool Passed{Check(Assets.Initialize(), "Mesh Picking assets initialize")};
        UWorld World{};
        World.Initialize();
        const FAssetHandle Cube{Assets.FindAsset(FAssetPath{"/Game/System/Mesh/Cube.bin"})};
        AActor* Actor{World.SpawnStaticMeshActor(Cube, FVector3{})};
        USceneComponent* Parent{Actor->AddComponent<USceneComponent>()};
        UStaticMeshComponent* Mesh{static_cast<UStaticMeshComponent*>(Actor->GetRootComponent())};
        Mesh->AttachToComponent(Parent);
        World.GetPickingSubsystem().RebuildAccelerationStructure();
        float Distance{};
        const FRay Initial{FVector3{0.0f, 0.0f, -10.0f}.ToSimpleMath(), FVector3::UnitZ.ToSimpleMath()};
        Passed = Check(Mesh->RaycastMesh(Initial, Distance, 100.0f) && Hit(World, *Mesh), "Mesh and world Picking warm their transform consumers") && Passed;
        Parent->SetRelativeLocation(FVector3{20.0f, 0.0f, 0.0f});
        const FRay Moved{FVector3{20.0f, 0.0f, -10.0f}.ToSimpleMath(), FVector3::UnitZ.ToSimpleMath()};
        Passed = Check(!Mesh->RaycastMesh(Initial, Distance, 100.0f) && Mesh->RaycastMesh(Moved, Distance, 100.0f) && Hit(World, *Mesh) && BoundsMatch(*Mesh), "Parent mutation refreshes both mesh raycast cache and world Picking without a preceding Transform query") && Passed;
        World.CleanupWorld();
        return Passed;
    }
}

int main() {
    const std::filesystem::path Repository{std::filesystem::current_path()};
    const std::filesystem::path Sandbox{Repository / "Build/Diagnostics/TransformHierarchyFixtures" / FGuid::NewGuid().ToString().c_str()};
    std::filesystem::create_directories(Sandbox / "Content");

    for (const char* Directory : {"System", "Shader", "Pipeline", "Font", "Texture"}) {
        std::filesystem::copy(Repository / "Content" / Directory, Sandbox / "Content" / Directory, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    }

    std::filesystem::copy(Repository / "scenes", Sandbox / "scenes", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    std::filesystem::current_path(Sandbox);
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    UEngine Engine{};
    Engine.Initialize();
    UTransformProbe::StaticClass();
    bool Passed{RunValues()};
    Passed = RunHierarchy() && Passed;
    Passed = RunAbsoluteOptions() && Passed;
    Passed = RunArchives() && Passed;
    Passed = RunRotationEditing() && Passed;
    Passed = RunMeshPicking() && Passed;
    Engine.Shutdown();
    CoUninitialize();
    std::filesystem::current_path(Repository);
    return Passed ? 0 : 1;
}
