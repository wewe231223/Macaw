#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "World/Component/UBillboardTextComponent.h"
#include "RenderCore/FTextSceneProxy.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UFont.h"
#include "Asset/FTextGeometry.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/AActor.h"
#include "World/UWorld.h"

void UBillboardTextComponent::SetFontHandle(FAssetHandle InFontHandle) {
    mFontReference.Assign(InFontHandle, FAssetRegistry::TryGet());
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetPipelineHandle(FAssetHandle InPipelineHandle) {
    mPipelineReference.Assign(InPipelineHandle, FAssetRegistry::TryGet());
    MarkRenderStateDirty();
}

void UBillboardTextComponent::SetText(const FString& InText) {
    if (mText == InText) {
        return;
    }

    mText = InText;
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetColor(const FVector4& InColor) {
    if (mColor == InColor) {
        return;
    }

    mColor = InColor;
    MarkRenderStateDirty();
}

void UBillboardTextComponent::SetCharacterHeight(float InCharacterHeight) {
    const float NewHeight{std::max(InCharacterHeight, 0.001f)};

    if (mCharacterHeight == NewHeight) {
        return;
    }

    mCharacterHeight = NewHeight;
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetLetterSpacing(float InLetterSpacing) {
    if (mLetterSpacing == InLetterSpacing) {
        return;
    }

    mLetterSpacing = InLetterSpacing;
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetLineSpacing(float InLineSpacing) {
    if (mLineSpacing == InLineSpacing) {
        return;
    }

    mLineSpacing = InLineSpacing;
    RebuildTextGeometry();
}

FAssetHandle UBillboardTextComponent::GetFontHandle() const {
    return mFontReference.GetHandle();
}

FAssetHandle UBillboardTextComponent::GetPipelineHandle() const {
    return mPipelineReference.GetHandle();
}

FAssetHandle UBillboardTextComponent::GetRenderFontHandle() const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    return Registry != nullptr && Registry->ResolveAsset<UFont>(GetFontHandle()) == nullptr ? Registry->FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"}) : GetFontHandle();
}

FAssetHandle UBillboardTextComponent::GetRenderPipelineHandle() const {
    const IAssetRegistry* Registry{FAssetRegistry::TryGet()};

    return Registry != nullptr && Registry->ResolveAsset<UPipeline>(GetPipelineHandle()) == nullptr ? Registry->FindAsset(FAssetPath{"/Game/Pipeline/Text.json"}) : GetPipelineHandle();
}

const FString& UBillboardTextComponent::GetText() const {
    return mText;
}

const FVector4& UBillboardTextComponent::GetColor() const {
    return mColor;
}

float UBillboardTextComponent::GetCharacterHeight() const {
    return mCharacterHeight;
}

float UBillboardTextComponent::GetLetterSpacing() const {
    return mLetterSpacing;
}

float UBillboardTextComponent::GetLineSpacing() const {
    return mLineSpacing;
}

const TArray<FTextVertex>& UBillboardTextComponent::GetVertices() const {
    return mVertices;
}

void UBillboardTextComponent::RebuildTextGeometry() {
    mVertices.clear();

    FAssetRegistry* Registry{FAssetRegistry::TryGet()};
    const FAssetHandle FontHandle{GetRenderFontHandle()};
    const UFont* Font{Registry != nullptr ? Registry->ResolveAsset<UFont>(FontHandle) : nullptr};

    if (Font != nullptr) {
        BuildTextGeometry(*Font, *Registry, FontHandle, mText, mCharacterHeight, mLetterSpacing, mLineSpacing, mVertices);
    }

    MarkRenderStateDirty();
}

void UBillboardTextComponent::OnRegister() {
    Super::OnRegister();

    const IAssetResolver* Resolver{FAssetRegistry::TryGet()};

    mFontReference.Resolve(Resolver);
    mPipelineReference.Resolve(Resolver);

    RebuildTextGeometry();
}

void UBillboardTextComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        RebuildTextGeometry();
    }
}

bool UBillboardTextComponent::TryGetTextWorld(FMatrix& OutWorld) const {
    OutWorld = GetComponentToWorld();

    return true;
}

bool UBillboardTextComponent::ShouldCreateRenderState() const {
    return IsVisible();
}

std::unique_ptr<FPrimitiveSceneProxy> UBillboardTextComponent::CreateSceneProxy() const {
    const AActor* Owner{GetOwner()};
    FTextDrawData Data{};

    Data.mFontHandle = GetRenderFontHandle();
    Data.mPipelineHandle = GetRenderPipelineHandle();

    if (!IsRegistered() || !IsVisible() || Owner == nullptr || !Data.mFontHandle || !Data.mPipelineHandle || mVertices.empty() || !TryGetTextWorld(Data.mWorld)) {
        return nullptr;
    }

    Data.mColor = mColor;
    Data.mVertices = mVertices;

    FPrimitiveTransform Transform{GetRenderTransform()};

    Transform.mWorld = Data.mWorld;

    return std::make_unique<FTextSceneProxy>(GetHandle(), Owner->GetHandle(), Transform, std::move(Data));
}

void UBillboardTextComponent::SendRenderTransform() {
    FPrimitiveTransform Transform{GetRenderTransform()};

    if (GetBelongingWorld() != nullptr && IsRenderStateCreated() && TryGetTextWorld(Transform.mWorld)) {
        GetBelongingWorld()->GetRenderSubsystem().UpdatePrimitiveTransform(GetHandle(), Transform);
    }
}

UBillboardTextComponent::UBillboardTextComponent() {
    InitializeClass(StaticClass());
}

void UBillboardTextComponent::SerializeFontHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UBillboardTextComponent& Object{*static_cast<UBillboardTextComponent*>(Container)};

    SerializeAssetReference(Archive, "FontAssetGuid", "FontAssetPath", Object.mFontReference);
}

void UBillboardTextComponent::SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UBillboardTextComponent& Object{*static_cast<UBillboardTextComponent*>(Container)};

    SerializeAssetReference(Archive, "PipelineAssetGuid", "PipelineAssetPath", Object.mPipelineReference);
}
