#include "pch.h"
#include "Editor/Panel/FPropertyEditorContext.h"
#include "Editor/View/ILineRenderer.h"
#include "Asset/FAssetRegistry.h"
#include "Asset/UMaterial.h"
#include "Asset/UTexture.h"
#include "Asset/UFont.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include <array>
#include <cfloat>
#include <memory>
#include <vector>
#include "World/Component/UActorComponent.h"
#include "World/Component/UBillboardComponent.h"
#include "World/Component/UBillboardTextComponent.h"
#include "World/Component/UBoxColliderComponent.h"
#include "World/Component/UCameraComponent.h"
#include "World/Component/UCollisionComponent.h"
#include "World/Component/UDirectionalLightComponent.h"
#include "World/Component/ULightComponent.h"
#include "World/Component/ULightComponentBase.h"
#include "World/Component/ULocalLightComponent.h"
#include "World/Component/UMeshComponent.h"
#include "World/Component/UNameTagComponent.h"
#include "World/Component/UPointLightComponent.h"
#include "World/Component/UPrimitiveComponent.h"
#include "World/Component/USceneComponent.h"
#include "World/Component/UScrollUVComponent.h"
#include "World/Component/USpotLightComponent.h"
#include "World/Component/UStaticMeshComponent.h"
#include "World/Component/USubUVComponent.h"

namespace {
constexpr float MinimumConeAngle{0.0f};
constexpr float MaximumConeAngle{89.9f};
constexpr char BasePipelinePath[]{"/Game/Pipeline/Base"};
constexpr char TextureBasePipelinePath[]{"/Game/Pipeline/TexturedBase.json"};
}

void UActorComponent::DrawPanels(FPropertyEditorContext& Context) {
    Context.DrawBool("Active", IsActive(), [this](bool BActive) {
        SetActive(BActive);
    });
}

void UBillboardComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPrimitiveComponent::DrawPanels(Context);
    Context.DrawColor("Color", GetColor(), [this](const FVector4& NewColor) {
        SetColor(NewColor);
    });

    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    FAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    Context.DrawAssetPicker("Texture", *Registry, *UTexture::StaticTypeInfo(), GetTextureHandle(), [this](FAssetHandle NewHandle) {
        SetTextureHandle(NewHandle);
    });
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle NewHandle) {
        SetPipelineHandle(NewHandle);
    });
}

void UBillboardTextComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPrimitiveComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Billboard Text")) {
        return;
    }

    Context.DrawText("Text", GetText(), [this](const FString& NewText) {
        SetText(NewText);
    });
    Context.DrawColor("Color", GetColor(), [this](const FVector4& NewColor) {
        SetColor(NewColor);
    });
    Context.DrawFloat("Character Height", GetCharacterHeight(), 0.01f, 0.001f, 1000.0f, [this](float NewHeight) {
        SetCharacterHeight(NewHeight);
    });
    Context.DrawFloat("Letter Spacing", GetLetterSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {
        SetLetterSpacing(NewSpacing);
    });
    Context.DrawFloat("Line Spacing", GetLineSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {
        SetLineSpacing(NewSpacing);
    });

    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    FAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry == nullptr) {
        Context.DrawDisabledText("Font/Pipeline: Asset registry unavailable");
        return;
    }

    Context.DrawAssetPicker("Font", *Registry, *UFont::StaticTypeInfo(), GetFontHandle(), [this](FAssetHandle NewHandle) {
        SetFontHandle(NewHandle);
    });
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle NewHandle) {
        SetPipelineHandle(NewHandle);
    });
}

void UBoxColliderComponent::DrawEditorBounds(ILineRenderer& LineRenderer, ELineDepthMode DepthMode) const {
    DirectX::BoundingOrientedBox WorldBox{};
    mObb.Transform(WorldBox, GetComponentToWorld().ToSimpleMath());

    std::array<DirectX::XMFLOAT3, DirectX::BoundingOrientedBox::CORNER_COUNT> Corners{};
    WorldBox.GetCorners(Corners.data());

    const FVector4 LineColor{FVector4{1.0f, 1.0f, 0.0f, 1.0f}};
    const float Thickness{1.0f};
    const auto AddEdge{[&LineRenderer, &Corners, LineColor, Thickness, DepthMode](std::size_t Start, std::size_t End) {
        LineRenderer.AddLine(FVector3{Corners[Start]}, FVector3{Corners[End]}, LineColor, Thickness, DepthMode);
    }};

    AddEdge(0, 1);
    AddEdge(1, 2);
    AddEdge(2, 3);
    AddEdge(3, 0);
    AddEdge(4, 5);
    AddEdge(5, 6);
    AddEdge(6, 7);
    AddEdge(7, 4);
    AddEdge(0, 4);
    AddEdge(1, 5);
    AddEdge(2, 6);
    AddEdge(3, 7);
}

void UBoxColliderComponent::DrawPanels(FPropertyEditorContext& Context) {
    UCollisionComponent::DrawPanels(Context);
    Context.DrawVector3("Extent", GetExtent(), 0.05f, 0.001f, FLT_MAX, [this](const FVector3& Extent) {
        SetExtent(Extent);
    });

    AActor* Actor{GetOwner()};
    if (Actor == nullptr) {
        return;
    }
    UMeshComponent* CurrentMesh{GetMeshComponent()};
    const char* Preview{CurrentMesh != nullptr ? CurrentMesh->GetTypeInfo()->mTypeName.data() : "None"};
    std::vector<FPropertyReferenceOption> Candidates{};
    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};
        if (CandidateComponent == nullptr || !CandidateComponent->GetTypeInfo()->IsA<UMeshComponent>()) {
            continue;
        }

        auto* Mesh{static_cast<UMeshComponent*>(CandidateComponent)};
        Candidates.push_back({Mesh, FString{Mesh->GetTypeInfo()->mTypeName}, Mesh == CurrentMesh, [this, Mesh] {
                                  SetMeshComponent(Mesh);
                              }});
    }
    Context.DrawReferencePicker("Source Mesh Component", Preview, CurrentMesh == nullptr, [this] {
        SetMeshComponent(nullptr);
    },
                                Candidates);
    Context.DrawButton("Build Bounds From Mesh", [this] {
        BuildBoundsFromMesh();
    });
}

void UCameraComponent::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);
    Context.DrawFloat("FOV (Degrees)", DirectX::XMConvertToDegrees(GetFOV()), 0.1f, 1.0f, 179.0f, [this](float FOVDegrees) {
        SetFOV(DirectX::XMConvertToRadians(FOVDegrees));
    });
    Context.DrawFloat("Aspect Ratio", GetAspectRatio(), 0.01f, 0.01f, 100.0f, [this](float AspectRatio) {
        SetAspectRatio(AspectRatio);
    });
    Context.DrawFloat("Near Plane", GetNearPlane(), 0.01f, 0.001f, GetFarPlane() - 0.001f, [this](float NearPlane) {
        SetNearPlane(NearPlane);
    });
    Context.DrawFloat("Far Plane", GetFarPlane(), 1.0f, GetNearPlane() + 0.001f, 1000000.0f, [this](float FarPlane) {
        SetFarPlane(FarPlane);
    });
}

void UCollisionComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPrimitiveComponent::DrawPanels(Context);
    Context.DrawBool("Collision Enabled", IsCollisionEnabled(), [this](bool BEnabled) {
        SetCollisionEnabled(BEnabled);
    });
}

void ULightComponentBase::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Light")) {
        return;
    }

    Context.DrawColor("Color", FVector4{mLightColor, 1.0f}, [this](const FVector4& Color) {
        SetLightColor(FVector3{Color.mX, Color.mY, Color.mZ});
    });
    Context.DrawFloat("Intensity", GetIntensity(), 0.1f, 0.0f, FLT_MAX, [this](float InIntensity) {
        SetIntensity(InIntensity);
    });
    Context.DrawBool("Visible", IsVisible(), [this](bool BInVisible) {
        SetVisible(BInVisible);
    });
}

void ULocalLightComponent::DrawPanels(FPropertyEditorContext& Context) {
    ULightComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Local Light")) {
        return;
    }

    Context.DrawFloat("Attenuation Radius", GetAttenuationRadius(), 1.0f, 0.0f, FLT_MAX, [this](float InAttenuationRadius) {
        SetAttenuationRadius(InAttenuationRadius);
    });
}

void UMeshComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPrimitiveComponent::DrawPanels(Context);
    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    FAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};
    if (Registry == nullptr) {
        Context.DrawDisabledText("Mesh: Asset registry unavailable");
        return;
    }
    Context.DrawAssetPicker("Mesh", *Registry, *UMesh::StaticTypeInfo(), GetMeshHandle(), [this](FAssetHandle Handle) {
        SetMeshHandle(Handle);
    });
}

void UNameTagComponent::DrawPanels(FPropertyEditorContext& Context) {
    if (!Context.BeginCategory("Name Tag")) {
        return;
    }

    Context.DrawColor("Color", GetColor(), [this](const FVector4& NewColor) {
        SetColor(NewColor);
    });
    Context.DrawFloat("Character Height", GetCharacterHeight(), 0.01f, 0.001f, 1000.0f, [this](float NewHeight) {
        SetCharacterHeight(NewHeight);
    });
    Context.DrawFloat("Letter Spacing", GetLetterSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {
        SetLetterSpacing(NewSpacing);
    });
    Context.DrawFloat("Line Spacing", GetLineSpacing(), 0.01f, -100.0f, 100.0f, [this](float NewSpacing) {
        SetLineSpacing(NewSpacing);
    });

    AActor* Owner{GetOwner()};
    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    FAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry == nullptr) {
        Context.DrawDisabledText("Font/Pipeline: Asset registry unavailable");
        return;
    }

    Context.DrawAssetPicker("Font", *Registry, *UFont::StaticTypeInfo(), GetFontHandle(), [this](FAssetHandle NewHandle) {
        SetFontHandle(NewHandle);
    });
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle NewHandle) {
        SetPipelineHandle(NewHandle);
    });
}

void UPrimitiveComponent::DrawPanels(FPropertyEditorContext& Context) {
    USceneComponent::DrawPanels(Context);
    Context.DrawBool("Visible", IsVisible(), [this](bool BVisible) {
        SetVisible(BVisible);
    });
}

void USceneComponent::DrawPanels(FPropertyEditorContext& Context) {
    UActorComponent::DrawPanels(Context);

    if (Context.BeginCategory("Transform")) {
        Context.DrawTransform("Relative Transform", GetRelativeTransform(), [this](const FTransform& Transform) {
            SetRelativeTransform(Transform);
        });
    }

    AActor* Actor{GetOwner()};
    if (Actor == nullptr || !Context.BeginCategory("Attachment")) {
        return;
    }
    if (Actor->GetRootComponent() == this) {
        Context.DrawDisabledText("Root Component");
        return;
    }

    USceneComponent* CurrentParent{GetParent()};
    const char* Preview{CurrentParent != nullptr ? CurrentParent->GetTypeInfo()->mTypeName.data() : "None"};
    std::vector<FPropertyReferenceOption> Candidates{};
    for (const std::unique_ptr<UActorComponent>& Candidate : Actor->GetComponents()) {
        UActorComponent* CandidateComponent{Candidate.get()};
        if (CandidateComponent == nullptr || !CandidateComponent->GetTypeInfo()->IsA<USceneComponent>()) {
            continue;
        }

        auto* Parent{static_cast<USceneComponent*>(CandidateComponent)};
        if (Parent == this)
            continue;

        Candidates.push_back({Parent, FString{Parent->GetTypeInfo()->mTypeName}, Parent == CurrentParent, [this, Parent] {
                                  AttachToComponent(Parent, EAttachmentTransformRule::KeepWorldTransform);
                              }});
    }
    Context.DrawReferencePicker("Parent", Preview, CurrentParent == nullptr, [this] {
        DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
    },
                                Candidates);
    Context.DrawButton("Make Root Component", [this, Actor] {
        DetachFromComponent(EAttachmentTransformRule::KeepWorldTransform);
        Actor->SetRootComponent(this);
    });
}

void UScrollUVComponent::DrawPanels(FPropertyEditorContext& Context) {
    UBillboardComponent::DrawPanels(Context);

    Context.DrawVector2("ScrollSpeed", mScrollSpeed, 0.01f, -5.0f, 5.0f, [this](FVector2 NewSpeed) {
        SetScrollSpeed(NewSpeed);
    });
}

void USpotLightComponent::DrawPanels(FPropertyEditorContext& Context) {
    UPointLightComponent::DrawPanels(Context);

    if (!Context.BeginCategory("Spot Light")) {
        return;
    }

    Context.DrawFloat("Inner Cone Angle", GetInnerConeAngle(), 0.1f, MinimumConeAngle, GetOuterConeAngle(), [this](float InInnerConeAngle) {
        SetInnerConeAngle(InInnerConeAngle);
    });
    Context.DrawFloat("Outer Cone Angle", GetOuterConeAngle(), 0.1f, GetInnerConeAngle(), MaximumConeAngle, [this](float InOuterConeAngle) {
        SetOuterConeAngle(InOuterConeAngle);
    });
}

void UStaticMeshComponent::DrawPanels(FPropertyEditorContext& Context) {
    UMeshComponent::DrawPanels(Context);
    AActor* Owner{GetOwner()};

    UWorld* World{Owner != nullptr ? Owner->GetWorld() : nullptr};
    FAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry == nullptr) {
        Context.DrawDisabledText("Material/Pipeline: Asset registry unavailable");
        return;
    }
    Context.DrawAssetPicker("Material", *Registry, *UMaterial::StaticTypeInfo(), GetMaterialHandle(), [this, Registry](FAssetHandle Handle) {
        SetMaterialHandle(Handle);

        const UMaterial* Material{Registry->ResolveAsset<UMaterial>(GetMaterialHandle())};
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

        const FAssetHandle DesiredPipelineHandle{Registry->FindAsset(FAssetPath{HasTexture ? TextureBasePipelinePath : BasePipelinePath})};
        if (DesiredPipelineHandle != GetPipelineHandle() && Registry->ResolveAsset<UPipeline>(DesiredPipelineHandle) != nullptr) {
            SetPipelineHandle(DesiredPipelineHandle);
        }
    });
    Context.DrawAssetPicker("Pipeline", *Registry, *UPipeline::StaticTypeInfo(), GetPipelineHandle(), [this](FAssetHandle Handle) {
        SetPipelineHandle(Handle);
    });
}

void USubUVComponent::DrawPanels(FPropertyEditorContext& Context) {
    UBillboardComponent::DrawPanels(Context);

    Context.DrawFloat("FrameRate", mFrameRate, 1.0f, 0.0f, 240.0f, [this](float NewRate) {
        SetFrameRate(NewRate);
    });
    Context.DrawVector2("SubImage", FVector2{static_cast<float>(mSubImageHorizontal), static_cast<float>(mSubImageVertical)}, 1.0f, 1.0f, 100.0f, [this](const FVector2& NewValue) {
        SetSubImage(static_cast<Int32>(NewValue.mX), static_cast<Int32>(NewValue.mY), mTotalFrame, mFrameRate, mBLooping);
    });
}
