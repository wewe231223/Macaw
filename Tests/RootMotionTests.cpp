#include "Asset/FAssetFile.h"
#include "Asset/FAssetManager.h"
#include "Asset/USkeletalMesh.h"
#include "Asset/USkeleton.h"
#include "Asset/Animation/UAnimGraph.h"
#include "Asset/Animation/UAnimSequence.h"
#include "Asset/Animation/UBlendSpace1D.h"
#include "Core/Math/FRotator.h"
#include "CoreUObject/FObjectDuplicationContext.h"
#include "CoreUObject/Reflection/Cast.h"
#include "CoreUObject/Reflection/NewObject.h"
#include "Engine/ASkeletalMeshActor.h"
#include "Engine/Animation/FAnimNode_BlendSpacePlayer.h"
#include "Engine/Animation/FAnimNode_SequencePlayer.h"
#include "Engine/Animation/FAnimNode_StateMachine.h"
#include "Engine/Animation/FAnimNode_TwoWayBlend.h"
#include "Engine/Animation/FAnimationRuntime.h"
#include "Engine/Animation/UAnimSingleNodeInstance.h"
#include "Engine/Component/USkeletalMeshComponent.h"
#include "Engine/Component/USceneComponent.h"
#include "Engine/EPrimitiveType.h"
#include "Engine/FWorldCloner.h"
#include "Engine/FWorldContext.h"
#include "Engine/PrimitiveMesh.h"
#include "Engine/SceneSerializer.h"
#include "Engine/UWorld.h"
#include "Serialization/MemoryArchive.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

    class FTestAnimInstance final : public Engine::UAnimInstance {
    public:
        void SetRoot(Engine::FAnimNode_Base* Root);
    };

    void FTestAnimInstance::SetRoot(Engine::FAnimNode_Base* Root) {
        mRootNode = Root;
    }

    void Require(bool Condition, const Core::FString& Message) {
        if (!Condition) {
            throw std::runtime_error{Message};
        }

        std::cout << "PASS " << Message << '\n';
    }

    bool Near(float A, float B) {
        return std::abs(A - B) < 0.0002f;
    }

    bool Near(const Core::FVector& A, const Core::FVector& B) {
        return (A - B).Length() < 0.0003f;
    }

    bool Identity(const Core::FTransform& Transform) {
        return Near(Transform.mLocation, {}) && Transform.mRotation.Equals(Core::FQuat{});
    }

    std::unique_ptr<Asset::UAnimSequence> MakeSequence(float Distance, Core::Uint32 Frames = 4, const Core::FQuat& EndRotation = {}) {
        Asset::FAnimDataModel Model{};

        Model.SetFrameRate({4, 1});
        Model.SetNumberOfFrames(Frames);

        Asset::FBoneAnimationTrack Child{};

        Child.mName = "Child";
        Child.mInternalTrack.mPosKeys.push_back({0.0f, 0.0f, 1.0f});
        Child.mInternalTrack.mRotKeys.push_back({});
        Child.mInternalTrack.mScaleKeys.push_back({1.0f, 1.0f, 1.0f});
        Model.GetBoneAnimationTracks().push_back(std::move(Child));

        Asset::FBoneAnimationTrack Root{};

        Root.mName = "Root";
        Root.mInternalTrack.mScaleKeys.push_back({1.0f, 1.0f, 1.0f});

        for (Core::Uint32 Key{}; Key <= Frames; ++Key) {
            const float Phase{static_cast<float>(Key) / static_cast<float>(Frames)};

            Root.mInternalTrack.mPosKeys.push_back({3.0f + Phase * Distance, 0.0f, 0.0f});
            Root.mInternalTrack.mRotKeys.push_back(Core::FQuat{}.Slerp(EndRotation, Phase));
        }

        Model.GetBoneAnimationTracks().push_back(std::move(Root));

        auto Sequence{CoreUObject::NewObject<Asset::UAnimSequence>()};
        Core::FString Error{};

        Require(Sequence->SetDataModel(std::move(Model), Error), "Create animation fixture " + Error);
        Sequence->SetRootMotionEnabled(true);

        return Sequence;
    }

    void WriteSequenceFixture(Asset::FAssetManager& Assets, const Asset::UAnimSequence& Sequence, const std::filesystem::path& Path, Core::Uint32 Version, Core::Uint32 Lock) {
        Core::TArray<Core::Uint8> Payload{};
        Serialization::FMemoryArchive Archive{Payload};
        Asset::FAnimDataModel Model{Sequence.GetDataModel()};
        Core::Uint32 Numerator{Model.GetFrameRate().mNumerator};
        Core::Uint32 Denominator{Model.GetFrameRate().mDenominator};
        Core::Uint32 Frames{Model.GetNumberOfFrames()};
        Core::Uint32 Tracks{static_cast<Core::Uint32>(Model.GetBoneAnimationTracks().size())};

        Archive.Serialize("AnimSequenceVersion", Version);
        Archive.Serialize("FrameRateNumerator", Numerator);
        Archive.Serialize("FrameRateDenominator", Denominator);
        Archive.Serialize("NumberOfFrames", Frames);
        Archive.BeginArray("Tracks", Tracks);

        for (Asset::FBoneAnimationTrack& Track : Model.GetBoneAnimationTracks()) {
            Archive.Serialize("Name", Track.mName);

            const auto WriteChannel{[&Archive](auto& Channel) {
                Core::Uint32 Count{static_cast<Core::Uint32>(Channel.size())};

                Archive.BeginArray("Channel", Count);

                for (auto& Key : Channel) {
                    Archive.Serialize("Key", Key);
                }

                Archive.EndArray();
            }};

            WriteChannel(Track.mInternalTrack.mPosKeys);
            WriteChannel(Track.mInternalTrack.mRotKeys);
            WriteChannel(Track.mInternalTrack.mScaleKeys);
        }

        Archive.EndArray();

        if (Version == 2) {
            bool Enabled{true};
            bool Forced{};
            Core::FString RootBone{};

            Archive.Serialize("EnableRootMotion", Enabled);
            Archive.Serialize("RootMotionRootLock", Lock);
            Archive.Serialize("ForceRootLock", Forced);
            Archive.Serialize("RootMotionBone", RootBone);
        }

        Core::FString Error{};

        Require(!Archive.HasError() && Asset::FAssetFile::Save(Assets.GetContentRoot() / Path, Asset::EAssetType::AnimSequence, Payload, Error), "Write asset format fixture " + Error);
    }

    void TestExtraction(Asset::UAnimSequence& Sequence) {
        Require(Near(Sequence.ExtractRootMotion(1, 0.0, 0.25, false).mLocation.x, 1.0f), "Extract a forward interval by track index");
        Require(Near(Sequence.ExtractRootMotion(1, 0.8, 0.4, true).mLocation.x, 1.6f), "Preserve movement across a loop boundary");
        Require(Near(Sequence.ExtractRootMotion(1, 0.8, 3.25, true).mLocation.x, 13.0f), "Preserve movement across multiple complete loops");
        Require(Near(Sequence.ExtractRootMotion(1, 0.2, -2.5, true).mLocation.x, -10.0f), "Extract multiple reverse loops");
        Require(Near(Sequence.ExtractRootMotion(1, 0.8, 2.0, false).mLocation.x, 0.8f), "Nonlooping extraction clamps at the final key");
        Require(Near(Sequence.ExtractRootMotion(1, 1.0, -0.25, true).mLocation.x, -1.0f), "Reverse playback can start at the end");
        Require(Identity(Sequence.ExtractRootMotion(-1, 0.0, 0.5, true)) && Identity(Sequence.ExtractRootMotion(1, 0.0, std::numeric_limits<double>::infinity(), true)), "Missing tracks and invalid intervals produce identity");
    }

    void TestModes(Engine::USkeletalMeshComponent& Mesh, Asset::UAnimSequence& Sequence) {
        Engine::UAnimSingleNodeInstance Instance{};

        Instance.InitializeAnimation(&Mesh);
        Instance.SetAnimation(&Sequence);
        Instance.Play(true);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 4.0f) && Identity(Instance.ConsumeExtractedRootMotion()), "NoRootMotionExtraction preserves the animated root and yields no actor motion");

        Instance.SetPosition(0.0);
        Instance.SetRootMotionMode(Engine::ERootMotionMode::IgnoreRootMotion);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 3.0f) && Identity(Instance.ConsumeExtractedRootMotion()), "IgnoreRootMotion locks the root and discards movement");

        Instance.SetPosition(0.0);
        Instance.SetRootMotionMode(Engine::ERootMotionMode::RootMotionFromEverything);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 3.0f) && Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 1.0f), "RootMotionFromEverything locks and extracts using the root name mapping");
        Require(Identity(Instance.ConsumeExtractedRootMotion()), "Root motion is consumed once");

        Instance.UpdateAnimation(0.25f);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 2.0f), "Manual consumption accumulates successive updates");
        Instance.Stop();
        Instance.UpdateAnimation(0.25f);
        Require(Identity(Instance.ConsumeExtractedRootMotion()), "Stopped playback clears previous movement");

        Instance.SetPosition(0.7);
        Instance.Play(true);
        Instance.UpdateAnimation(0.1f);
        Require(Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 0.4f), "Seeking does not extract the skipped interval");
        Instance.UpdateAnimation(0.0f);
        Require(Identity(Instance.ConsumeExtractedRootMotion()), "Zero delta does not reuse an old root delta");

        Sequence.SetRootMotionRootLock(Asset::ERootMotionRootLock::RefPose);
        Instance.UpdateAnimation(0.0f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 2.0f), "RefPose root lock uses the target skeleton");
        Sequence.SetRootMotionRootLock(Asset::ERootMotionRootLock::Zero);
        Instance.UpdateAnimation(0.0f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 0.0f), "Zero root lock uses the mesh origin");
        Sequence.SetRootMotionEnabled(false);
        Instance.UpdateAnimation(0.0f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 6.2f), "An asset without enabled root motion remains animated");
        Sequence.SetForceRootLock(true);
        Instance.SetRootMotionMode(Engine::ERootMotionMode::NoRootMotionExtraction);
        Instance.UpdateAnimation(0.0f);
        Require(Near(Instance.GetPose()[0].mLocation.x, 0.0f) && Identity(Instance.ConsumeExtractedRootMotion()), "ForceRootLock works independently of extraction mode and asset enablement");

        Sequence.SetForceRootLock(false);
        Sequence.SetRootMotionEnabled(true);
        Sequence.SetRootMotionRootLock(Asset::ERootMotionRootLock::AnimFirstFrame);
    }

    void TestBlending(Engine::USkeletalMeshComponent& Mesh, Asset::UAnimSequence& Walk, Asset::UAnimSequence& Run) {
        FTestAnimInstance Instance{};
        Engine::FAnimNode_SequencePlayer A{};
        Engine::FAnimNode_SequencePlayer B{};
        Engine::FAnimNode_TwoWayBlend Blend{};

        A.mSequence = &Walk;
        B.mSequence = &Run;
        Blend.mA.mLinkedNode = &A;
        Blend.mB.mLinkedNode = &B;
        Blend.mAlpha = 0.25f;
        Instance.SetRoot(&Blend);
        Instance.InitializeAnimation(&Mesh);
        Instance.SetRootMotionMode(Engine::ERootMotionMode::RootMotionFromEverything);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 1.25f), "TwoWayBlend applies pose weights to root motion");

        Blend.mB.mLinkedNode = &A;
        Blend.Initialize(Instance);
        Instance.UpdateAnimation(0.25f);
        Require(Near(static_cast<float>(A.GetAccumulatedTime()), 0.25f) && Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 1.0f), "A shared player advances and contributes once per weighted path");

        auto Space{CoreUObject::NewObject<Asset::UBlendSpace1D>()};
        Engine::FAnimNode_BlendSpacePlayer Player{};

        Space->AddSample(&Walk, {0.0f, 0.0f, 0.0f});
        Space->AddSample(&Run, {1.0f, 0.0f, 0.0f});
        Player.mBlendSpace = Space.get();
        Player.mX = 0.5f;
        Instance.SetRoot(&Player);
        Instance.InitializeAnimation(&Mesh);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 5.0f / 3.0f), "BlendSpace samples different clip lengths at the same phase");

        Run.SetRootMotionEnabled(false);
        Player.Initialize(Instance);
        Instance.UpdateAnimation(0.25f);
        Require(Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 1.0f / 3.0f), "Disabled clips retain their blend weight without contributing root motion");
        Run.SetRootMotionEnabled(true);

        auto Graph{CoreUObject::NewObject<Asset::UAnimGraph>()};
        Asset::FAnimStateMachineData Data{};
        const Core::Int32 Source{Data.AddState("Walk", 0.0f, 0.0f, *Graph)};
        const Core::Int32 Target{Data.AddState("Run", 100.0f, 0.0f, *Graph)};
        const Core::Int32 Transition{Data.AddTransition(Source, Target)};
        Asset::FAnimTransitionData* Rule{Data.FindTransition(Transition)};
        const Core::Int32 Condition{Rule->mRule.AddNode(Asset::EAnimRuleNodeType::Bool, 0.0f, 0.0f)};

        Rule->mDuration = 1.0f;
        Rule->mRule.FindNode(Condition)->mBoolValue = true;
        Rule->mRule.SetResult(Condition);

        Engine::FAnimNode_StateMachine Machine{};

        Machine.Configure(Data);
        Machine.AddState(*Data.FindState(Source), &A);
        Machine.AddState(*Data.FindState(Target), &B);
        Instance.SetRoot(&Machine);
        Instance.InitializeAnimation(&Mesh);
        Instance.UpdateAnimation(0.25f);
        Instance.ConsumeExtractedRootMotion();
        Instance.UpdateAnimation(0.25f);
        Require(Near(Machine.GetBlendAlpha(), 0.25f) && Near(Instance.ConsumeExtractedRootMotion().mLocation.x, 1.25f), "StateMachine transitions blend root motion with the current transition alpha");
    }

    void TestRotation(Engine::USkeletalMeshComponent& Mesh) {
        const Core::FQuat QuarterTurn{Core::FRotator{0.0f, 90.0f, 0.0f}};
        auto Sequence{MakeSequence(0.0f, 4, QuarterTurn)};
        Engine::UAnimSingleNodeInstance Instance{};

        Instance.InitializeAnimation(&Mesh);
        Instance.SetRootMotionMode(Engine::ERootMotionMode::RootMotionFromEverything);
        Instance.SetAnimation(Sequence.get());
        Instance.Play(false);
        Instance.UpdateAnimation(1.0f);

        const Core::FTransform Delta{Instance.ConsumeExtractedRootMotion()};
        const Core::FVector Pivot{3.0f, 0.0f, 0.0f};

        Require(Near(Delta.mLocation + Delta.mRotation.RotateVector(Pivot), Pivot), "Extracted root rotation preserves an offset lock pivot");

        Core::FTransform Component{{10.0f, 5.0f, 0.0f}, QuarterTurn, {2.0f, 2.0f, 2.0f}};
        Core::FTransform Actor{{8.0f, 4.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}};
        const Core::FTransform Moved{Engine::FAnimationRuntime::ApplyRootMotionToTransform(Delta, Component, Actor)};
        const Core::FVector WorldPivot{Component.ToMatrix().TransformPosition(Pivot)};
        const Core::FQuat WorldRotation{Moved.mRotation * Actor.mRotation.Inverse()};
        const Core::FVector MovedComponent{Moved.mLocation + WorldRotation.RotateVector(Component.mLocation - Actor.mLocation)};
        const Core::FVector MovedPivot{MovedComponent + WorldRotation.RotateVector(WorldPivot - Component.mLocation)};

        Require(Near(MovedPivot, WorldPivot), "World conversion preserves the pivot with mesh offset, scale and facing");

        Instance.SetPlayRate(-1.0f);
        Instance.SetPosition(1.0);
        Instance.Play(false);
        Instance.UpdateAnimation(1.0f);
        const Core::FTransform Reverse{Instance.ConsumeExtractedRootMotion()};
        Require(Identity(Engine::FAnimationRuntime::AccumulateRootMotion(Delta, Reverse)), "Reverse root rotation cancels forward root rotation");
    }

    void TestAssets(Asset::FAssetManager& Assets, Asset::UAnimSequence& Sequence) {
        Core::FString Error{};

        Sequence.SetForceRootLock(true);
        Sequence.SetRootMotionRootLock(Asset::ERootMotionRootLock::RefPose);
        Sequence.SetRootMotionBone("Root");
        Require(Assets.SaveAnimSequence("Settings.masset", Sequence, Error), "Save sequence root motion settings " + Error);

        Asset::FAssetManager Reloaded{Assets.GetContentRoot()};
        const Asset::UAnimSequence* Loaded{Reloaded.LoadAnimSequence("Settings.masset", Error)};

        Require(Loaded != nullptr && Loaded->IsRootMotionEnabled() && Loaded->IsRootLockForced() && Loaded->GetRootMotionRootLock() == Asset::ERootMotionRootLock::RefPose && Loaded->GetRootMotionBone() == "Root", "Sequence settings survive an asset round trip");
        WriteSequenceFixture(Assets, Sequence, "Legacy.masset", 1, 0);

        const Asset::UAnimSequence* Legacy{Assets.LoadAnimSequence("Legacy.masset", Error)};

        Require(Legacy != nullptr && !Legacy->IsRootMotionEnabled() && !Legacy->IsRootLockForced(), "Version 1 animation assets load with root motion disabled");
        WriteSequenceFixture(Assets, Sequence, "Invalid.masset", 2, 99);
        Require(Assets.LoadAnimSequence("Invalid.masset", Error) == nullptr, "Malformed root lock settings are rejected");
        Sequence.SetForceRootLock(false);
        Sequence.SetRootMotionRootLock(Asset::ERootMotionRootLock::AnimFirstFrame);
        Sequence.SetRootMotionBone("");
    }

}

int main() {
    try {
        const std::filesystem::path Content{std::filesystem::absolute("Build/RootMotionTests/Content")};

        std::filesystem::create_directories(Content);

        Asset::FAssetManager Assets{Content};
        Asset::USkeleton Skeleton{};
        Core::FString Error{};
        const Core::FTransform RootBind{{2.0f, 0.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}};

        Require(Skeleton.SetBones({{"Root", -1, RootBind.ToMatrix()}, {"Child", 0, Core::FTransform{{0.0f, 0.0f, 1.0f}, {}, {1.0f, 1.0f, 1.0f}}.ToMatrix()}}), "Create a target skeleton");
        Require(Assets.SaveSkeleton("Skeleton.masset", Skeleton, Error), "Save target skeleton " + Error);

        Asset::USkeletalMesh MeshAsset{};
        Asset::FMeshData Geometry{Engine::BuildPrimitive(Engine::EPrimitiveType::Cube)};
        Core::TArray<Asset::FMeshImportSkinWeight> Weights{};

        Weights.assign(Geometry.GetPositions().size(), Asset::FMeshImportSkinWeight::FromInfluences({{0, 1.0f}}));
        Require(MeshAsset.SetMeshData(std::move(Geometry), std::move(Weights)) && Assets.SaveSkeletalMesh("Mesh.masset", MeshAsset, Error), "Save a synthetic skeletal mesh " + Error);

        Engine::FWorldContext Context{Engine::EWorldType::Game};

        Context.GetWorld()->SetAssets(&Assets);

        auto* Actor{CoreUObject::Cast<Engine::ASkeletalMeshActor>(Context.GetWorld()->SpawnActor(Engine::ASkeletalMeshActor::StaticClass()))};

        Require(Actor != nullptr, "Spawn a skeletal actor without opening the editor");

        Engine::USkeletalMeshComponent* Mesh{Actor->GetSkeletalMeshComponent()};

        Require(Mesh->SetSkeletonPath("Skeleton.masset") && Mesh->SetMeshPath("Mesh.masset"), "Resolve mesh and skeleton fixtures");

        auto Walk{MakeSequence(4.0f)};
        auto Run{MakeSequence(16.0f, 8)};

        TestExtraction(*Walk);
        TestModes(*Mesh, *Walk);
        TestBlending(*Mesh, *Walk, *Run);
        TestRotation(*Mesh);
        TestAssets(Assets, *Walk);

        Mesh->SetSingleNodeRootMotionMode(Engine::ERootMotionMode::RootMotionFromEverything);
        Require(Context.GetWorld()->BeginPlay(), "Begin a headless game world");
        Mesh->PlayAnimation(Walk.get(), true);

        auto Child{CoreUObject::NewObject<Engine::USceneComponent>()};

        Child->SetTransform({{0.0f, 2.0f, 0.0f}, {}, {1.0f, 1.0f, 1.0f}});
        Engine::USceneComponent* ChildComponent{Child.get()};
        Require(Actor->AddComponent(std::move(Child)), "Add a second spatial component");
        Context.GetWorld()->Tick(0.25f);
        Require(Near(Actor->GetActorTransform().mLocation.x, 1.0f) && Near(ChildComponent->GetTransform().mLocation, {1.0f, 2.0f, 0.0f}), "Automatic application moves the actor and its other components together");
        Require(Identity(Mesh->ConsumeRootMotion()), "Automatic application leaves no duplicate motion to consume");
        Mesh->SetApplyRootMotionToActor(false);
        Context.GetWorld()->Tick(0.25f);
        Require(Near(Actor->GetActorTransform().mLocation.x, 1.0f) && Near(Mesh->ConsumeRootMotion().mLocation.x, 1.0f) && Identity(Mesh->ConsumeRootMotion()), "Manual application exposes movement once without moving the actor");
        Require(!Mesh->SetRootMotionModeValue(3) && Mesh->GetRootMotionModeValue() == 2, "Script mode bridge rejects unsupported Montage modes");

        Require(Mesh->SetAnimClass("UAnimGraphInstance"), "Configure an AnimInstance class for scene serialization");
        Mesh->SetRootMotionMode(Engine::ERootMotionMode::RootMotionFromEverything);
        Context.GetWorld()->EndPlay();

        const std::filesystem::path ScenePath{Content / "RootMotionScene.masset"};

        Require(Engine::FSceneSerializer::Save(ScenePath, &Context, Error), "Save root motion scene configuration " + Error);

        auto Restored{Engine::FSceneSerializer::Load(ScenePath, Error, &Assets)};

        Require(Restored != nullptr, "Reload root motion scene configuration " + Error);

        CoreUObject::FObjectDuplicationContext Duplication{};
        auto Clone{Engine::FWorldCloner::Clone(Restored.get(), Engine::EWorldType::PIE, &Duplication, Error)};

        Require(Clone != nullptr, "Clone root motion configuration " + Error);
        auto* ClonedActor{CoreUObject::Cast<Engine::ASkeletalMeshActor>(Clone->GetWorld()->FindActor(Actor->GetName()))};
        Require(ClonedActor != nullptr && ClonedActor->GetSkeletalMeshComponent()->GetRootMotionMode() == Engine::ERootMotionMode::RootMotionFromEverything && !ClonedActor->GetSkeletalMeshComponent()->IsRootMotionAppliedToActor(), "Scene cloning preserves AnimInstance mode and the application policy");
        Context.GetWorld()->EndPlay();

        std::cout << "Root motion tests passed\n";

        return 0;
    } catch (const std::exception& Error) {
        std::cerr << "FAIL " << Error.what() << '\n';

        return 1;
    }
}
