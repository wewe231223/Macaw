#include "pch.h"
#include "World/Component/UNameTagComponent.h"
#include "Asset/FTextGeometry.h"
#include "World/Component/UMeshComponent.h"
#include "World/UWorld.h"
#include "CoreUObject/UObjectSystem.h"

#include <algorithm>

void UNameTagComponent::SetTargetActor(AActor* InTargetActor) {
    if (InTargetActor == nullptr || InTargetActor == GetOwner()) {
        mTargetActor.Reset();
        mExplicitTargetGuid = {};
    } else {
        mTargetActor.Set(InTargetActor);
        mExplicitTargetGuid = InTargetActor->GetGuid();
    }

    RefreshGuidText();
}

AActor* UNameTagComponent::GetTargetActor() const {
    return mExplicitTargetGuid.IsValid() ? mTargetActor.Get() : GetOwner();
}

void UNameTagComponent::SetTargetLocalOffset(const FVector3& InOffset) {
    mTargetLocalOffset = InOffset;
}

const FVector3& UNameTagComponent::GetTargetLocalOffset() const {
    return mTargetLocalOffset;
}

const FVector3& UNameTagComponent::GetObjectOffset() const {
    return mTargetLocalOffset;
}

FGuid UNameTagComponent::GetObjectGuid() const {
    const AActor* Owner{GetOwner()};

    return mExplicitTargetGuid.IsValid() ? mExplicitTargetGuid : Owner != nullptr ? Owner->GetGuid() : FGuid{};
}

void UNameTagComponent::SetFontHandle(FAssetHandle FontHandle) {
    if (mFontHandle == FontHandle) {
        return;
    }

    mFontHandle = FontHandle;

    const UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    mFontAssetPath = Registry != nullptr && Registry->GetAssetPath(FontHandle) != nullptr ? *Registry->GetAssetPath(FontHandle) : FAssetPath{};
    mFontAssetGuid = Registry != nullptr && Registry->GetAssetGuid(FontHandle) != nullptr ? *Registry->GetAssetGuid(FontHandle) : FGuid{};
    RebuildTextGeometry();
}

FAssetHandle UNameTagComponent::GetFontHandle() const {
    return mFontHandle;
}

void UNameTagComponent::SetText(const FString& Text) {
    if (mText != Text) {
        mText = Text;
        RebuildTextGeometry();
    }
}

const FString& UNameTagComponent::GetText() const {
    return mText;
}

void UNameTagComponent::SetColor(const FVector4& Color) {
    mColor = Color;
}

const FVector4& UNameTagComponent::GetColor() const {
    return mColor;
}

void UNameTagComponent::SetPixelHeight(float PixelHeight) {
    if (std::isfinite(PixelHeight) && PixelHeight > 0.0f && mPixelHeight != PixelHeight) {
        mPixelHeight = PixelHeight;
        RebuildTextGeometry();
    }
}

float UNameTagComponent::GetPixelHeight() const {
    return mPixelHeight;
}

void UNameTagComponent::SetLetterSpacing(float LetterSpacing) {
    if (std::isfinite(LetterSpacing) && mLetterSpacing != LetterSpacing) {
        mLetterSpacing = LetterSpacing;
        RebuildTextGeometry();
    }
}

float UNameTagComponent::GetLetterSpacing() const {
    return mLetterSpacing;
}

void UNameTagComponent::SetLineSpacing(float LineSpacing) {
    if (std::isfinite(LineSpacing) && mLineSpacing != LineSpacing) {
        mLineSpacing = LineSpacing;
        RebuildTextGeometry();
    }
}

float UNameTagComponent::GetLineSpacing() const {
    return mLineSpacing;
}

void UNameTagComponent::SetScreenOffset(const FVector2& ScreenOffset) {
    if (std::isfinite(ScreenOffset.mX) && std::isfinite(ScreenOffset.mY)) {
        mScreenOffset = ScreenOffset;
    }
}

const FVector2& UNameTagComponent::GetScreenOffset() const {
    return mScreenOffset;
}

void UNameTagComponent::SetVisible(bool Visible) {
    mVisible = Visible;
}

bool UNameTagComponent::IsVisible() const {
    return mVisible;
}

bool UNameTagComponent::MakeOverlayText(FOverlayTextProbe& OutProbe) const {
    const AActor* Target{GetTargetActor()};

    if (!IsRegistered() || !mVisible || !mFontHandle || mVertices.empty() || Target == nullptr || Target->GetRootComponent() == nullptr || Target->GetWorld() != GetBelongingWorld()) {
        return false;
    }

    const FMatrix TargetWorld{Target->GetActorTransform().ToMatrixWithScale()};
    const FVector3 Origin{TargetWorld.Translation()};
    FVector3 Minimum{Origin};
    FVector3 Maximum{Origin};
    bool HasBounds{};

    for (const std::unique_ptr<UActorComponent>& Component : Target->GetComponents()) {
        if (!Component->GetTypeInfo()->IsA<UMeshComponent>()) {
            continue;
        }

        const UMeshComponent* Mesh{static_cast<const UMeshComponent*>(Component.get())};

        if (!Mesh->IsRegistered() || !Mesh->IsVisible() || !Mesh->GetMeshHandle()) {
            continue;
        }

        DirectX::BoundingOrientedBox Box{};
        DirectX::XMFLOAT3 Corners[DirectX::BoundingOrientedBox::CORNER_COUNT]{};

        Mesh->GetPickingBox().Transform(Box, Mesh->GetComponentToWorld().ToSimpleMath());
        Box.GetCorners(Corners);

        for (const DirectX::XMFLOAT3& Corner : Corners) {
            const FVector3 Position{Corner};

            if (!HasBounds) {
                Minimum = Position;
                Maximum = Position;
                HasBounds = true;
            } else {
                Minimum.mX = std::min(Minimum.mX, Position.mX);
                Minimum.mY = std::min(Minimum.mY, Position.mY);
                Minimum.mZ = std::min(Minimum.mZ, Position.mZ);
                Maximum.mX = std::max(Maximum.mX, Position.mX);
                Maximum.mY = std::max(Maximum.mY, Position.mY);
                Maximum.mZ = std::max(Maximum.mZ, Position.mZ);
            }
        }
    }

    const FVector3 Center{HasBounds ? (Minimum + Maximum) * 0.5f : Origin};
    const FVector3 Offset{TargetWorld.TransformPosition(mTargetLocalOffset) - Origin};

    OutProbe.mWorldAnchor = Center + Offset;
    OutProbe.mWorldBoundsExtent = HasBounds ? (Maximum - Minimum) * 0.5f : FVector3{};
    OutProbe.mScreenOffset = mScreenOffset;
    OutProbe.mFontHandle = mFontHandle;
    OutProbe.mColor = mColor;
    OutProbe.mVertices = mVertices;

    return true;
}

void UNameTagComponent::OnRegister() {
    USceneComponent::OnRegister();

    UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    if (Registry != nullptr && Registry->ResolveAsset<UFont>(mFontHandle) == nullptr) {
        mFontHandle = Registry->FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"});
    }

    if (World != nullptr) {
        World->GetOverlaySubsystem().RegisterComponent(this);
    }

    RefreshGuidText();
    RebuildTextGeometry();
}

void UNameTagComponent::OnUnregister() {
    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetOverlaySubsystem().UnregisterComponent(this);
    }

    USceneComponent::OnUnregister();
}

void UNameTagComponent::Serialize(FArchive& Archive) {
    USceneComponent::Serialize(Archive);

    const IAssetResolver* Registry{Archive.GetAssetResolver()};

    if (Archive.IsSaving() && Registry != nullptr) {
        if (const FAssetPath* Path{Registry->GetAssetPath(mFontHandle)}) {
            mFontAssetPath = *Path;
        }

        if (const FGuid* Guid{Registry->GetAssetGuid(mFontHandle)}) {
            mFontAssetGuid = *Guid;
        }
    }

    Archive.Serialize("bVisible", mVisible);
    Archive.Serialize("FontAssetGuid", mFontAssetGuid);
    Archive.Serialize("FontAssetPath", mFontAssetPath.mPath);
    Archive.Serialize("Text", mText);
    Archive.Serialize("Color", mColor);
    Archive.Serialize("PixelHeight", mPixelHeight);
    Archive.Serialize("LetterSpacing", mLetterSpacing);
    Archive.Serialize("LineSpacing", mLineSpacing);
    Archive.Serialize("ScreenOffset", mScreenOffset);
    Archive.Serialize("TargetActorGuid", mExplicitTargetGuid);
    Archive.Serialize("TargetLocalOffset", mTargetLocalOffset);

    if (Archive.IsLoading()) {
        mTargetActor.Reset();
        mFontHandle = Registry != nullptr ? Registry->FindAsset(mFontAssetGuid) : FAssetHandle{};

        if (!mFontHandle && Registry != nullptr) {
            mFontHandle = Registry->FindAsset(mFontAssetPath);
        }

        mPixelHeight = std::isfinite(mPixelHeight) && mPixelHeight > 0.0f ? mPixelHeight : 24.0f;
        mLetterSpacing = std::isfinite(mLetterSpacing) ? mLetterSpacing : 0.0f;
        mLineSpacing = std::isfinite(mLineSpacing) ? mLineSpacing : 0.0f;
        mScreenOffset = std::isfinite(mScreenOffset.mX) && std::isfinite(mScreenOffset.mY) ? mScreenOffset : FVector2{0.0f, -8.0f};
        RebuildTextGeometry();
    }
}

bool UNameTagComponent::ResolveLoadedReferences() {
    if (!USceneComponent::ResolveLoadedReferences()) {
        return false;
    }

    if (!mExplicitTargetGuid.IsValid()) {
        return GetOwner() != nullptr;
    }

    UObject* Object{UObjectSystem::Resolve(UObjectSystem::FindHandleByGuid(mExplicitTargetGuid))};

    if (Object == nullptr || !Object->GetTypeInfo()->IsA<AActor>()) {
        return false;
    }

    mTargetActor.Set(static_cast<AActor*>(Object));

    return true;
}

void UNameTagComponent::RefreshGuidText() {
    const FGuid Guid{GetObjectGuid()};

    SetText(Guid.IsValid() ? Guid.ToString() : FString{});
}

void UNameTagComponent::RebuildTextGeometry() {
    mVertices.clear();

    UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};
    IAssetRegistryMutator* Mutator{World != nullptr ? World->GetAssetRegistryMutator() : nullptr};
    const UFont* Font{Registry != nullptr ? Registry->ResolveAsset<UFont>(mFontHandle) : nullptr};

    if (Font != nullptr && Mutator != nullptr) {
        BuildTextGeometry(*Font, *Mutator, mFontHandle, mText, mPixelHeight, mLetterSpacing, mLineSpacing, mVertices);
    }
}
