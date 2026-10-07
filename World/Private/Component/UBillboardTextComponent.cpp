#include "pch.h"
#include "World/Component/UBillboardTextComponent.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UFont.h"
#include "Asset/FTextGeometry.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/AActor.h"
#include "World/UWorld.h"
#include "World/Subsystem/UTextSubsystem.h"

void UBillboardTextComponent::SetFontHandle(FAssetHandle InFontHandle) {
    if (mFontHandle == InFontHandle) {
        return;
    }

    mFontHandle = InFontHandle;

    UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* AssetRegistry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    mFontAssetPath = AssetRegistry != nullptr && AssetRegistry->GetAssetPath(mFontHandle) != nullptr ? *AssetRegistry->GetAssetPath(mFontHandle) : FAssetPath{};
    mFontAssetGuid = AssetRegistry != nullptr && AssetRegistry->GetAssetGuid(mFontHandle) != nullptr ? *AssetRegistry->GetAssetGuid(mFontHandle) : FGuid{};
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetPipelineHandle(FAssetHandle InPipelineHandle) {
    mPipelineHandle = InPipelineHandle;

    UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* AssetRegistry{World != nullptr ? World->GetAssetRegistry() : nullptr};

    mPipelineAssetPath = AssetRegistry != nullptr && AssetRegistry->GetAssetPath(mPipelineHandle) != nullptr ? *AssetRegistry->GetAssetPath(mPipelineHandle) : FAssetPath{};
    mPipelineAssetGuid = AssetRegistry != nullptr && AssetRegistry->GetAssetGuid(mPipelineHandle) != nullptr ? *AssetRegistry->GetAssetGuid(mPipelineHandle) : FGuid{};
}

void UBillboardTextComponent::SetText(const FString& InText) {
    if (mText == InText) {
        return;
    }

    mText = InText;
    RebuildTextGeometry();
}

void UBillboardTextComponent::SetColor(const FVector4& InColor) {
    mColor = InColor;
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
    return mFontHandle;
}

FAssetHandle UBillboardTextComponent::GetPipelineHandle() const {
    return mPipelineHandle;
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

bool UBillboardTextComponent::MakeTextRender(FTextProbe& OutProbe) const {
    if (!IsRegistered() || !IsVisible() || !mFontHandle || !mPipelineHandle || mVertices.empty()) {
        return false;
    }

    // UBillBoardComponent가 World Transform을 계산한다.
    if (!TryGetTextWorld(OutProbe.mWorld)) {
        return false;
    }

    OutProbe.mFontHandle = mFontHandle;
    OutProbe.mPipelineHandle = mPipelineHandle;
    OutProbe.mColor = mColor;
    OutProbe.mVertices = mVertices;

    return true;
}

void UBillboardTextComponent::RebuildTextGeometry() {
    mVertices.clear();

    UWorld* World{GetBelongingWorld()};
    const IAssetRegistry* Registry{World != nullptr ? World->GetAssetRegistry() : nullptr};
    IAssetRegistryMutator* Mutator{World != nullptr ? World->GetAssetRegistryMutator() : nullptr};
    const UFont* Font{Registry != nullptr ? Registry->ResolveAsset<UFont>(mFontHandle) : nullptr};

    if (Font != nullptr && Mutator != nullptr) {
        BuildTextGeometry(*Font, *Mutator, mFontHandle, mText, mCharacterHeight, mLetterSpacing, mLineSpacing, mVertices);
    }
}

void UBillboardTextComponent::OnRegister() {
    UPrimitiveComponent::OnRegister();

    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        const IAssetRegistry* AssetRegistry{World->GetAssetRegistry()};

        if (AssetRegistry != nullptr) {
            if (AssetRegistry->ResolveAsset<UFont>(mFontHandle) == nullptr) {
                mFontHandle = AssetRegistry->FindAsset(FAssetPath{"/Game/Font/NotoSansKR-Medium.ttf"});
            }

            if (AssetRegistry->ResolveAsset<UPipeline>(mPipelineHandle) == nullptr) {
                mPipelineHandle = AssetRegistry->FindAsset(FAssetPath{"/Game/Pipeline/Text.json"});
            }
        }

        World->GetTextSubsystem().RegisterComponent(this);
    }

    RebuildTextGeometry();
}

void UBillboardTextComponent::OnUnregister() {
    UWorld* World{GetBelongingWorld()};

    if (World != nullptr) {
        World->GetTextSubsystem().UnregisterComponent(this);
    }

    UPrimitiveComponent::OnUnregister();
}

void UBillboardTextComponent::Serialize(FArchive& Archive) {
    UPrimitiveComponent::Serialize(Archive);

    const IAssetResolver* AssetRegistry{Archive.GetAssetResolver()};

    if (Archive.IsSaving() && AssetRegistry != nullptr) {
        if (const FAssetPath* AssetPath{AssetRegistry->GetAssetPath(mFontHandle)}) {
            mFontAssetPath = *AssetPath;
        }

        if (const FGuid* AssetGuid{AssetRegistry->GetAssetGuid(mFontHandle)}) {
            mFontAssetGuid = *AssetGuid;
        }

        if (const FAssetPath* AssetPath{AssetRegistry->GetAssetPath(mPipelineHandle)}) {
            mPipelineAssetPath = *AssetPath;
        }

        if (const FGuid* AssetGuid{AssetRegistry->GetAssetGuid(mPipelineHandle)}) {
            mPipelineAssetGuid = *AssetGuid;
        }
    }

    Archive.Serialize("FontAssetGuid", mFontAssetGuid);
    Archive.Serialize("FontAssetPath", mFontAssetPath.mPath);
    Archive.Serialize("PipelineAssetGuid", mPipelineAssetGuid);
    Archive.Serialize("PipelineAssetPath", mPipelineAssetPath.mPath);

    if (Archive.IsLoading()) {
        mFontHandle = AssetRegistry != nullptr ? AssetRegistry->FindAsset(mFontAssetGuid) : FAssetHandle{};

        if (!mFontHandle && AssetRegistry != nullptr) {
            mFontHandle = AssetRegistry->FindAsset(mFontAssetPath);
        }

        mPipelineHandle = AssetRegistry != nullptr ? AssetRegistry->FindAsset(mPipelineAssetGuid) : FAssetHandle{};

        if (!mPipelineHandle && AssetRegistry != nullptr) {
            mPipelineHandle = AssetRegistry->FindAsset(mPipelineAssetPath);
        }
    }

    Archive.Serialize("Text", mText);
    Archive.Serialize("Color", mColor);
    Archive.Serialize("CharacterHeight", mCharacterHeight);
    Archive.Serialize("LetterSpacing", mLetterSpacing);
    Archive.Serialize("LineSpacing", mLineSpacing);
}

bool UBillboardTextComponent::TryGetTextWorld(FMatrix& OutWorld) const {
    OutWorld = GetComponentToWorld();

    return true;
}
