#include "pch.h"
#include "Editor/Property/FComponentDetails.h"
#include "Editor/Property/FReflectedDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "CoreUObject/Reflection/Cast.h"
#include "World/AActor.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/USubUVComponent.h"
#include "World/Component/UStaticMeshComponent.h"

void FComponentDetails::Draw(UActorComponent& Component, IPropertyEditorContext& Context) {
    FReflectedDetails::Draw(Component, Context, [&Component, &Context](const FString& Category) {
        if (Category == "Attachment") {
            if (USceneComponent* Scene{Cast<USceneComponent>(&Component)}) {
                DrawAttachmentActions(*Scene, Context);
            }
        } else if (Category == "Rendering") {
            if (UStaticMeshComponent* Mesh{Cast<UStaticMeshComponent>(&Component)}) {
                if (const UMesh* Asset{Mesh->ResolveMesh()}; Asset != nullptr) {
                    for (const FMeshMaterialSlot& Slot : Asset->GetMaterialSlots()) {
                        const FString Label{Slot.mName.ToString()};

                        Context.DrawAssetPicker(Label.c_str(), *UMaterial::StaticClass(), Mesh->GetMaterialOverride(Slot.mName), [Mesh, Name = Slot.mName](FAssetHandle Material) {
                            Mesh->SetMaterialOverride(Name, Material);
                        });
                    }
                }
            }
        } else if (Category == "Collision") {
            if (UBoxColliderComponent* Collider{Cast<UBoxColliderComponent>(&Component)}) {
                Context.DrawButton("Build Bounds From Mesh", [Collider] {
                    Collider->BuildBoundsFromMesh();
                });
            }
        } else if (Category == "UV Animation") {
            if (USubUVComponent* Animation{Cast<USubUVComponent>(&Component)}) {
                DrawPlaybackActions(*Animation, Context);
            }
        }
    });
}

void FComponentDetails::DrawAttachmentActions(USceneComponent& Component, IPropertyEditorContext& Context) {
    AActor* Actor{Component.GetOwner()};

    if (Actor == nullptr) {
        return;
    }

    if (Actor->GetRootComponent() == &Component) {
        Context.DrawDisabledText("Root Component");
        return;
    }

    Context.DrawButton("Make Root Component", [&Component, Actor] {
        Component.DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        Actor->SetRootComponent(&Component);
    });
}

void FComponentDetails::DrawPlaybackActions(USubUVComponent& Component, IPropertyEditorContext& Context) {
    if (Component.GetTargetMaterialSlot().IsNone()) {
        Context.DrawDisabledText("Applies to all material slots.");
    }

    if (Component.HasTargetConflict()) {
        Context.DrawDisabledText("This material slot is already controlled by another UV component.");
    }

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
}
