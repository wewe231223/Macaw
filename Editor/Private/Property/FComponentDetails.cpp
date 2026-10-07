#include "pch.h"
#include "Editor/Property/FComponentDetails.h"
#include "Editor/Property/IPropertyEditorContext.h"
#include "World/UWorld.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include "Asset/UMaterial.h"
#include "Asset/Pipeline/UPipeline.h"
#include <cfloat>
#include "World/Component/UActorComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/ULightComponentBase.h"
#include "World/Component/ULocalLightComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UNameTagComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UScrollUVComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/USubUVComponent.h"

void FComponentDetails::Draw(UActorComponent& Component, IPropertyEditorContext& Context) {
    if (Component.GetTypeInfo()->IsA(USpotLightComponent::StaticTypeInfo())) {
        DrawSpotLightComponent(static_cast<USpotLightComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UBoxColliderComponent::StaticTypeInfo())) {
        DrawBoxColliderComponent(static_cast<UBoxColliderComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(ULocalLightComponent::StaticTypeInfo())) {
        DrawLocalLightComponent(static_cast<ULocalLightComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UNameTagComponent::StaticTypeInfo())) {
        DrawNameTagComponent(static_cast<UNameTagComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UScrollUVComponent::StaticTypeInfo())) {
        DrawScrollUVComponent(static_cast<UScrollUVComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UStaticMeshComponent::StaticTypeInfo())) {
        DrawStaticMeshComponent(static_cast<UStaticMeshComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(USubUVComponent::StaticTypeInfo())) {
        DrawSubUVComponent(static_cast<USubUVComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UBillboardComponent::StaticTypeInfo())) {
        DrawBillboardComponent(static_cast<UBillboardComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UBillboardTextComponent::StaticTypeInfo())) {
        DrawBillboardTextComponent(static_cast<UBillboardTextComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UCollisionComponent::StaticTypeInfo())) {
        DrawCollisionComponent(static_cast<UCollisionComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UMeshComponent::StaticTypeInfo())) {
        DrawMeshComponent(static_cast<UMeshComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UCameraComponent::StaticTypeInfo())) {
        DrawCameraComponent(static_cast<UCameraComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(ULightComponentBase::StaticTypeInfo())) {
        DrawLightComponentBase(static_cast<ULightComponentBase&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UPrimitiveComponent::StaticTypeInfo())) {
        DrawPrimitiveComponent(static_cast<UPrimitiveComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(USceneComponent::StaticTypeInfo())) {
        DrawSceneComponent(static_cast<USceneComponent&>(Component), Context);
        return;
    }

    if (Component.GetTypeInfo()->IsA(UActorComponent::StaticTypeInfo())) {
        DrawActorComponent(static_cast<UActorComponent&>(Component), Context);
        return;
    }
}

void FComponentDetails::DrawActorComponent(UActorComponent& Component, IPropertyEditorContext& Context) {
    Context.DrawBool("Active", Component.IsActive(), [&Component](bool BActive) {
        Component.SetActive(BActive);
    });
}

void FComponentDetails::DrawBillboardComponent(UBillboardComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
    Context.DrawColor("Color", Component.GetColor(), [&Component](const FVector4& NewColor) {
        Component.SetColor(NewColor);
    });

    Context.DrawAssetPicker("Texture", *UTexture::StaticTypeInfo(), Component.GetTextureHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetTextureHandle(NewHandle);
    });

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticTypeInfo(), Component.GetPipelineHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetPipelineHandle(NewHandle);
    });
}

void FComponentDetails::DrawBillboardTextComponent(UBillboardTextComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);

    if (!Context.BeginCategory("Billboard Text")) {
        return;
    }

    Context.DrawText("Text", Component.GetText(), [&Component](const FString& NewText) {
        Component.SetText(NewText);
    });

    Context.DrawColor("Color", Component.GetColor(), [&Component](const FVector4& NewColor) {
        Component.SetColor(NewColor);
    });

    Context.DrawFloat("Character Height", Component.GetCharacterHeight(), 0.01f, 0.001f, 1000.0f, [&Component](float NewHeight) {
        Component.SetCharacterHeight(NewHeight);
    });

    Context.DrawFloat("Letter Spacing", Component.GetLetterSpacing(), 0.01f, -100.0f, 100.0f, [&Component](float NewSpacing) {
        Component.SetLetterSpacing(NewSpacing);
    });

    Context.DrawFloat("Line Spacing", Component.GetLineSpacing(), 0.01f, -100.0f, 100.0f, [&Component](float NewSpacing) {
        Component.SetLineSpacing(NewSpacing);
    });

    Context.DrawAssetPicker("Font", *UFont::StaticTypeInfo(), Component.GetFontHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetFontHandle(NewHandle);
    });

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticTypeInfo(), Component.GetPipelineHandle(), [&Component](FAssetHandle NewHandle) {
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
    const FString Preview{CurrentMesh != nullptr ? CurrentMesh->GetTypeInfo()->mTypeName.ToString() : FString{"None"}};
    std::vector<FPropertyReferenceOption> Candidates{};

    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};

        if (CandidateComponent == nullptr || !CandidateComponent->GetTypeInfo()->IsA<UMeshComponent>()) {
            continue;
        }

        auto* Mesh{static_cast<UMeshComponent*>(CandidateComponent)};
        Candidates.push_back({Mesh, Mesh->GetTypeInfo()->mTypeName.ToString(), Mesh == CurrentMesh, [&Component, Mesh] {
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

    Context.DrawFloat("FOV (Degrees)", DirectX::XMConvertToDegrees(Component.GetFOV()), 0.1f, 1.0f, 179.0f, [&Component](float FOVDegrees) {
        Component.SetFOV(DirectX::XMConvertToRadians(FOVDegrees));
    });

    Context.DrawFloat("Aspect Ratio", Component.GetAspectRatio(), 0.01f, 0.01f, 100.0f, [&Component](float AspectRatio) {
        Component.SetAspectRatio(AspectRatio);
    });

    Context.DrawFloat("Near Plane", Component.GetNearPlane(), 0.01f, 0.001f, Component.GetFarPlane() - 0.001f, [&Component](float NearPlane) {
        Component.SetNearPlane(NearPlane);
    });

    Context.DrawFloat("Far Plane", Component.GetFarPlane(), 1.0f, Component.GetNearPlane() + 0.001f, 1000000.0f, [&Component](float FarPlane) {
        Component.SetFarPlane(FarPlane);
    });
}

void FComponentDetails::DrawCollisionComponent(UCollisionComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
    Context.DrawBool("Collision Enabled", Component.IsCollisionEnabled(), [&Component](bool BEnabled) {
        Component.SetCollisionEnabled(BEnabled);
    });
}

void FComponentDetails::DrawLightComponentBase(ULightComponentBase& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);

    if (!Context.BeginCategory("Light")) {
        return;
    }

    Context.DrawColor("Color", FVector4{Component.GetLightColor(), 1.0f}, [&Component](const FVector4& Color) {
        Component.SetLightColor(FVector3{Color.mX, Color.mY, Color.mZ});
    });

    Context.DrawFloat("Intensity", Component.GetIntensity(), 0.1f, 0.0f, FLT_MAX, [&Component](float InIntensity) {
        Component.SetIntensity(InIntensity);
    });

    Context.DrawBool("Visible", Component.IsVisible(), [&Component](bool BInVisible) {
        Component.SetVisible(BInVisible);
    });
}

void FComponentDetails::DrawLocalLightComponent(ULocalLightComponent& Component, IPropertyEditorContext& Context) {
    DrawLightComponentBase(Component, Context);

    if (!Context.BeginCategory("Local Light")) {
        return;
    }

    Context.DrawFloat("Attenuation Radius", Component.GetAttenuationRadius(), 1.0f, 0.0f, FLT_MAX, [&Component](float InAttenuationRadius) {
        Component.SetAttenuationRadius(InAttenuationRadius);
    });
}

void FComponentDetails::DrawMeshComponent(UMeshComponent& Component, IPropertyEditorContext& Context) {
    DrawPrimitiveComponent(Component, Context);
    Context.DrawAssetPicker("Mesh", *UMesh::StaticTypeInfo(), Component.GetMeshHandle(), [&Component](FAssetHandle Handle) {
        Component.SetMeshHandle(Handle);
    });
}

void FComponentDetails::DrawNameTagComponent(UNameTagComponent& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);

    if (!Context.BeginCategory("Name Tag")) {
        return;
    }

    Context.DrawColor("Color", Component.GetColor(), [&Component](const FVector4& NewColor) {
        Component.SetColor(NewColor);
    });

    Context.DrawBool("Visible", Component.IsVisible(), [&Component](bool Visible) {
        Component.SetVisible(Visible);
    });

    Context.DrawFloat("Pixel Height", Component.GetPixelHeight(), 1.0f, 1.0f, 256.0f, [&Component](float NewHeight) {
        Component.SetPixelHeight(NewHeight);
    });

    Context.DrawVector2("Screen Offset", Component.GetScreenOffset(), 1.0f, -10000.0f, 10000.0f, [&Component](const FVector2& Offset) {
        Component.SetScreenOffset(Offset);
    });

    Context.DrawFloat("Letter Spacing", Component.GetLetterSpacing(), 0.01f, -100.0f, 100.0f, [&Component](float NewSpacing) {
        Component.SetLetterSpacing(NewSpacing);
    });

    Context.DrawFloat("Line Spacing", Component.GetLineSpacing(), 0.01f, -100.0f, 100.0f, [&Component](float NewSpacing) {
        Component.SetLineSpacing(NewSpacing);
    });

    Context.DrawAssetPicker("Font", *UFont::StaticTypeInfo(), Component.GetFontHandle(), [&Component](FAssetHandle NewHandle) {
        Component.SetFontHandle(NewHandle);
    });
}

void FComponentDetails::DrawPrimitiveComponent(UPrimitiveComponent& Component, IPropertyEditorContext& Context) {
    DrawSceneComponent(Component, Context);

    Context.DrawBool("Visible", Component.IsVisible(), [&Component](bool BVisible) {
        Component.SetVisible(BVisible);
    });
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
    const FString Preview{CurrentParent != nullptr ? CurrentParent->GetTypeInfo()->mTypeName.ToString() : FString{"None"}};
    std::vector<FPropertyReferenceOption> Candidates{};

    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};

        if (CandidateComponent == nullptr || !CandidateComponent->GetTypeInfo()->IsA<USceneComponent>()) {
            continue;
        }

        auto* Parent{static_cast<USceneComponent*>(CandidateComponent)};

        if (Parent == &Component)
            continue;

        Candidates.push_back({Parent, Parent->GetTypeInfo()->mTypeName.ToString(), Parent == CurrentParent, [&Component, Parent] {
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

void FComponentDetails::DrawScrollUVComponent(UScrollUVComponent& Component, IPropertyEditorContext& Context) {
    DrawBillboardComponent(Component, Context);

    Context.DrawVector2("ScrollSpeed", Component.GetScrollSpeed(), 0.01f, -5.0f, 5.0f, [&Component](FVector2 NewSpeed) {
        Component.SetScrollSpeed(NewSpeed);
    });
}

void FComponentDetails::DrawSpotLightComponent(USpotLightComponent& Component, IPropertyEditorContext& Context) {
    DrawLocalLightComponent(Component, Context);

    if (!Context.BeginCategory("Spot Light")) {
        return;
    }

    Context.DrawFloat("Inner Cone Angle", Component.GetInnerConeAngle(), 0.1f, 0.0f, Component.GetOuterConeAngle(), [&Component](float InInnerConeAngle) {
        Component.SetInnerConeAngle(InInnerConeAngle);
    });

    Context.DrawFloat("Outer Cone Angle", Component.GetOuterConeAngle(), 0.1f, Component.GetInnerConeAngle(), 89.9f, [&Component](float InOuterConeAngle) {
        Component.SetOuterConeAngle(InOuterConeAngle);
    });
}

void FComponentDetails::DrawStaticMeshComponent(UStaticMeshComponent& Component, IPropertyEditorContext& Context) {
    DrawMeshComponent(Component, Context);

    Context.DrawAssetPicker("Material", *UMaterial::StaticTypeInfo(), Component.GetMaterialHandle(), [&Component](FAssetHandle Handle) {
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

    Context.DrawAssetPicker("Pipeline", *UPipeline::StaticTypeInfo(), Component.GetPipelineHandle(), [&Component](FAssetHandle Handle) {
        Component.SetPipelineHandle(Handle);
    });
}

void FComponentDetails::DrawSubUVComponent(USubUVComponent& Component, IPropertyEditorContext& Context) {
    DrawBillboardComponent(Component, Context);

    Context.DrawFloat("FrameRate", Component.GetFrameRate(), 1.0f, 0.0f, 240.0f, [&Component](float NewRate) {
        Component.SetFrameRate(NewRate);
    });

    Context.DrawVector2("SubImage", FVector2{static_cast<float>(Component.GetSubImageHorizontal()), static_cast<float>(Component.GetSubImageVertical())}, 1.0f, 1.0f, 100.0f, [&Component](const FVector2& NewValue) {
        Component.SetSubImage(static_cast<Int32>(NewValue.mX), static_cast<Int32>(NewValue.mY), Component.GetTotalFrame(), Component.GetFrameRate(), Component.IsLooping());
    });
}
