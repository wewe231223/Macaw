#include "pch.h"
#include "CoreUObject/Reflection/UClass.h"
#include "CoreUObject/Reflection/FPropertyValue.h"
#include "Editor/Property/FComponentDetails.h"
#include "Editor/Property/FReflectedDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "World/UWorld.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include "Asset/UMaterial.h"
#include "Asset/Pipeline/UPipeline.h"
#include <cfloat>
#include <array>
#include "World/Component/UActorComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/ULightComponentBase.h"
#include "World/Component/ULocalLightComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UUVScrollComponent.h"
#include "World/Component/UFlipBookComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/USubUVComponent.h"

void FComponentDetails::Draw(UActorComponent& Component, IPropertyEditorContext& Context) {
    FReflectedDetails::Draw(Component, Context);

    if (Component.GetClass()->IsChildOf(USpotLightComponent::StaticClass())) {
        DrawSpotLightComponent(static_cast<USpotLightComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UBoxColliderComponent::StaticClass())) {
        DrawBoxColliderComponent(static_cast<UBoxColliderComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(ULocalLightComponent::StaticClass())) {
        DrawLocalLightComponent(static_cast<ULocalLightComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UUVScrollComponent::StaticClass())) {
        DrawUVScrollComponent(static_cast<UUVScrollComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UFlipBookComponent::StaticClass())) {
        DrawFlipBookComponent(static_cast<UFlipBookComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UStaticMeshComponent::StaticClass())) {
        DrawStaticMeshComponent(static_cast<UStaticMeshComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(USubUVComponent::StaticClass())) {
        DrawSubUVComponent(static_cast<USubUVComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UBillboardComponent::StaticClass())) {
        DrawBillboardComponent(static_cast<UBillboardComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UBillboardTextComponent::StaticClass())) {
        DrawBillboardTextComponent(static_cast<UBillboardTextComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UCollisionComponent::StaticClass())) {
        DrawCollisionComponent(static_cast<UCollisionComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UMeshComponent::StaticClass())) {
        DrawMeshComponent(static_cast<UMeshComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UCameraComponent::StaticClass())) {
        DrawCameraComponent(static_cast<UCameraComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(ULightComponentBase::StaticClass())) {
        DrawLightComponentBase(static_cast<ULightComponentBase&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UPrimitiveComponent::StaticClass())) {
        DrawPrimitiveComponent(static_cast<UPrimitiveComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(USceneComponent::StaticClass())) {
        DrawSceneComponent(static_cast<USceneComponent&>(Component), Context);
        return;
    }

    if (Component.GetClass()->IsChildOf(UActorComponent::StaticClass())) {
        DrawActorComponent(static_cast<UActorComponent&>(Component), Context);
        return;
    }
}

void FComponentDetails::DrawActorComponent(UActorComponent& Component, IPropertyEditorContext& Context) {
}

void FComponentDetails::DrawBillboardComponent(UBillboardComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
    Context.DrawAssetPicker("Texture", *UTexture::StaticClass(), Component.GetTextureHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetTextureHandle(NewHandle);
    });

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticClass(), Component.GetPipelineHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetPipelineHandle(NewHandle);
    });
}

void FComponentDetails::DrawBillboardTextComponent(UBillboardTextComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);

    if (!Context.BeginCategory("Text Assets")) {
        return;
    }

    Context.DrawAssetPicker("Font", *UFont::StaticClass(), Component.GetFontHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetFontHandle(NewHandle);
    });

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticClass(), Component.GetPipelineHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetPipelineHandle(NewHandle);
    });
}

void FComponentDetails::DrawBoxColliderComponent(UBoxColliderComponent& Component, IPropertyEditorContext& Context) {
    DrawCollisionComponent(Component, Context);
    Context.DrawVector3("Extent", Component.GetExtent(), 0.05f, 0.001f, FLT_MAX, [&Component](const FVector3& Extent) {
        Component.SetExtent(Extent);
    });

    AActor* Actor{Component.GetOwner()};

    if (Actor == nullptr) {
        return;
    }

    UMeshComponent* CurrentMesh{Component.GetMeshComponent()};
    const FString Preview{CurrentMesh != nullptr ? CurrentMesh->GetClass()->GetClassName().ToString() : FString{"None"}};
    std::vector<FPropertyReferenceOption> Candidates{};

    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};

        if (CandidateComponent == nullptr || !CandidateComponent->GetClass()->IsChildOf<UMeshComponent>()) {
            continue;
        }

        auto* Mesh{static_cast<UMeshComponent*>(CandidateComponent)};
        Candidates.push_back({Mesh, Mesh->GetClass()->GetClassName().ToString(), Mesh == CurrentMesh, [&Component, Mesh] {
            Component.SetMeshComponent(Mesh);
        }});
    }

    Context.DrawReferencePicker("Source Mesh Component", Preview.c_str(), CurrentMesh == nullptr, [&Component] {
        Component.SetMeshComponent(nullptr);
    }, Candidates);
    Context.DrawButton("Build Bounds From Mesh", [&Component] {
        Component.BuildBoundsFromMesh();
    });
}

void FComponentDetails::DrawCameraComponent(UCameraComponent& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);
}

void FComponentDetails::DrawCollisionComponent(UCollisionComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
}

void FComponentDetails::DrawLightComponentBase(ULightComponentBase& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);
}

void FComponentDetails::DrawLocalLightComponent(ULocalLightComponent& Component, IPropertyEditorContext& Context) {
    DrawLightComponentBase(Component, Context);
}

void FComponentDetails::DrawMeshComponent(UMeshComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
    Context.DrawAssetPicker("Mesh", *UMesh::StaticClass(), Component.GetMeshHandle(), [&Component](FAssetHandle Handle) {
        Component.SetMeshHandle(Handle);
    });
}

void FComponentDetails::DrawPrimitiveComponent(UPrimitiveComponent& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);

}

void FComponentDetails::DrawSceneComponent(USceneComponent& Component, IPropertyEditorContext& Context) {
    DrawActorComponent(Component, Context);

    if (Context.BeginCategory("Transform")) {
        Context.DrawTransform("Relative Transform", Component.GetRelativeTransform(), [&Component](const FTransform& Transform) {
            Component.SetRelativeTransform(Transform);
        });
    }

    AActor* Actor{Component.GetOwner()};

    if (Actor == nullptr || !Context.BeginCategory("Attachment")) {
        return;
    }

    if (Actor->GetRootComponent() == &Component) {
        Context.DrawDisabledText("Root Component");
        return;
    }

    USceneComponent* CurrentParent{Component.GetParent()};
    const FString Preview{CurrentParent != nullptr ? CurrentParent->GetClass()->GetClassName().ToString() : FString{"None"}};
    std::vector<FPropertyReferenceOption> Candidates{};

    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};

        if (CandidateComponent == nullptr || !CandidateComponent->GetClass()->IsChildOf<USceneComponent>()) {
            continue;
        }

        auto* Parent{static_cast<USceneComponent*>(CandidateComponent)};

        if (Parent == &Component)
            continue;

        Candidates.push_back({Parent, Parent->GetClass()->GetClassName().ToString(), Parent == CurrentParent, [&Component, Parent] {
            Component.AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform);
        }});
    }

    Context.DrawReferencePicker("Parent", Preview.c_str(), CurrentParent == nullptr, [&Component] {
        Component.DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
    }, Candidates);

    Context.DrawButton("Make Root Component", [&Component, Actor] {
        Component.DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        Actor->SetRootComponent(&Component);
    });
}

void FComponentDetails::DrawUVScrollComponent(UUVScrollComponent& Component, IPropertyEditorContext& Context) {
    DrawSubUVComponent(Component, Context);

}

void FComponentDetails::DrawSpotLightComponent(USpotLightComponent& Component, IPropertyEditorContext& Context) {
    DrawLocalLightComponent(Component, Context);
}

void FComponentDetails::DrawStaticMeshComponent(UStaticMeshComponent& Component, IPropertyEditorContext& Context) {
    DrawMeshComponent(Component, Context);

    Context.DrawAssetPicker("Material", *UMaterial::StaticClass(), Component.GetMaterialHandle(), [&Component](FAssetHandle Handle) {
        Component.SetMaterialHandle(Handle);

        AActor* Owner{Component.GetOwner()};
        UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
        const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

        if (Registry == nullptr) {
            return;
        }

        const UMaterial* Material{Registry->ResolveAsset<UMaterial>(Component.GetMaterialHandle())};
        bool HasTexture{};

        if (Material != nullptr) {
            for (Uint32 GroupIndex{}; GroupIndex < Material->GetGPUDataCount() && !HasTexture; ++GroupIndex) {
                const FMaterialChunkSignature Signature{Material->BuildChunkSignature(GroupIndex)};

                for (Uint8 TextureFieldIndex{}; TextureFieldIndex < Signature.mTextureFieldCount; ++TextureFieldIndex) {
                    if (Signature.GetTextureHandle(TextureFieldIndex)) {
                        HasTexture = true;
                        break;
                    }
                }
            }
        }

        const FAssetHandle DesiredPipelineHandle{Registry->FindAsset(FAssetPath{HasTexture ? "/Game/Pipeline/TexturedBase" : "/Game/Pipeline/Base"})};

        if (DesiredPipelineHandle != Component.GetPipelineHandle() && Registry->ResolveAsset<UPipeline>(DesiredPipelineHandle) != nullptr) {
            Component.SetPipelineHandle(DesiredPipelineHandle);
        }
    });

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticClass(), Component.GetPipelineHandle(), [&Component](FAssetHandle Handle) {
        Component.SetPipelineHandle(Handle);
    });
}

void FComponentDetails::DrawSubUVComponent(USubUVComponent& Component, IPropertyEditorContext& Context) {
    DrawActorComponent(Component, Context);

    UPrimitiveComponent* Target{Component.GetTargetComponent()};
    AActor* Owner{Component.GetOwner()};
    std::vector<FPropertyReferenceOption> Targets{};

    if (Owner != nullptr) {
        for (const std::unique_ptr<UActorComponent>& Candidate : Owner->GetComponents()) {
            if (!Candidate->IsBeingDestroyed() && (Candidate->GetClass()->IsChildOf(UStaticMeshComponent::StaticClass()) || Candidate->GetClass()->IsChildOf(UBillboardComponent::StaticClass()))) {
                UPrimitiveComponent* Primitive{static_cast<UPrimitiveComponent*>(Candidate.get())};

                Targets.push_back(FPropertyReferenceOption{Primitive, Primitive->GetName().ToString(), Primitive == Target, [&Component, Primitive] {
                    Component.SetTargetComponent(Primitive);
                }});
            }
        }
    }

    const FString TargetName{Target != nullptr ? Target->GetName().ToString() : "None"};

    Context.DrawReferencePicker("Target component", TargetName.c_str(), Target == nullptr, [&Component] {
        Component.SetTargetComponent(nullptr);
    }, Targets);

    const UMaterial* Material{nullptr};
    const IAssetRegistry* Registry{Owner != nullptr && Owner->GetWorld() != nullptr ? Owner->GetWorld()->GetAssetRegistry() : nullptr};

    if (Target != nullptr && Registry != nullptr && Target->GetClass()->IsChildOf(UStaticMeshComponent::StaticClass())) {
        Material = Registry->ResolveAsset<UMaterial>(static_cast<UStaticMeshComponent*>(Target)->GetMaterialHandle());
    }

    std::vector<FPropertyReferenceOption> Groups{};
    const FName CurrentGroup{Component.GetTargetMaterialGroup()};

    if (Material != nullptr) {
        for (Uint32 Index{}; Index < Material->GetGPUDataCount(); ++Index) {
            const FName Group{Material->GetGroupName(Index)};

            if (!Group.IsNone()) {
                Groups.push_back(FPropertyReferenceOption{reinterpret_cast<const void*>(static_cast<std::uintptr_t>(Index) + 1), Group.ToString(), Group == CurrentGroup, [&Component, Group] {
                    Component.SetTargetMaterialGroup(Group);
                }});
            }
        }
    }

    const FString GroupName{CurrentGroup.IsNone() ? "All material groups" : CurrentGroup.ToString()};

    Context.DrawReferencePicker("Material group", GroupName.c_str(), CurrentGroup.IsNone(), [&Component] {
        Component.SetTargetMaterialGroup({});
    }, Groups);

    if (CurrentGroup.IsNone()) {
        Context.DrawDisabledText("Applies to all material groups.");
    }

    if (Component.HasTargetConflict()) {
        Context.DrawDisabledText("This material group is already controlled by another UV component.");
    }

    Context.DrawBool("Looping", Component.IsLooping(), [&Component](bool Looping) {
        const std::array<FPropertyValue, 1> Arguments{FPropertyValue{Looping}};
        FPropertyValue Result{};

        Component.ProcessEvent(Component.FindFunction(FName{"SetLooping"}), Arguments, Result);
    });

    Context.DrawButton(Component.IsPlaying() ? "Pause" : "Play", [&Component] {
        FPropertyValue Result{};

        Component.ProcessEvent(Component.FindFunction(FName{Component.IsPlaying() ? "Pause" : "Play"}), {}, Result);
    });

    Context.DrawButton("Stop", [&Component] {
        FPropertyValue Result{};

        Component.ProcessEvent(Component.FindFunction(FName{"Stop"}), {}, Result);
    });

    Context.DrawButton("Restart", [&Component] {
        FPropertyValue Result{};

        Component.ProcessEvent(Component.FindFunction(FName{"Restart"}), {}, Result);
    });

    if (Component.GetClass() == USubUVComponent::StaticClass()) {
        Context.DrawVector2("UV scale", Component.GetUVTransform().mScale, 0.01f, -100.0f, 100.0f, [&Component](const FVector2& Scale) {
            Component.SetUVTransform(FUVTransform{Scale, Component.GetUVTransform().mOffset});
        });

        Context.DrawVector2("UV offset", Component.GetUVTransform().mOffset, 0.01f, -100.0f, 100.0f, [&Component](const FVector2& Offset) {
            Component.SetUVTransform(FUVTransform{Component.GetUVTransform().mScale, Offset});
        });
    }
}

void FComponentDetails::DrawFlipBookComponent(UFlipBookComponent& Component, IPropertyEditorContext& Context) {
    DrawSubUVComponent(Component, Context);

    Context.DrawVector2("SubImage", FVector2{static_cast<float>(Component.GetColumns()), static_cast<float>(Component.GetRows())}, 1.0f, 1.0f, 100.0f, [&Component](const FVector2& NewValue) {
        Component.SetSubImage(static_cast<Int32>(NewValue.mX), static_cast<Int32>(NewValue.mY), Component.GetTotalFrames(), Component.GetFrameRate(), Component.IsLooping());
    });

    Context.DrawFloat("Total frames", static_cast<float>(Component.GetTotalFrames()), 1.0f, 1.0f, static_cast<float>(static_cast<Int64>(Component.GetColumns()) * Component.GetRows()), [&Component](float Frames) {
        Component.SetSubImage(Component.GetColumns(), Component.GetRows(), static_cast<Int32>(Frames), Component.GetFrameRate(), Component.IsLooping());
    });

    Context.DrawFloat("Current frame", static_cast<float>(Component.GetCurrentFrame()), 1.0f, 0.0f, static_cast<float>(Component.GetTotalFrames() - 1), [&Component](float Frame) {
        Component.SetCurrentFrame(static_cast<Int32>(Frame));
    });
}
