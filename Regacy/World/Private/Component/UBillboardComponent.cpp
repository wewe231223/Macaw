#include "pch.h"
#include "Asset/FAssetRegistry.h"
#include "CoreUObject/Reflection/NativeSerialization.h"
#include "World/Component/UBillboardComponent.h"
#include "RenderCore/FBillboardSceneProxy.h"
#include "CoreUObject/Asset/IAssetRegistry.h"
#include "Asset/UTexture.h"
#include "Asset/Pipeline/UPipeline.h"
#include "World/AActor.h"
#include "World/UWorld.h"

bool UBillboardComponent::CanRenderBillBoard() const {
    return IsRegistered() && IsVisible();
}

void UBillboardComponent::OnRegister() {
    Super::OnRegister();

    const IAssetResolver* Resolver{FAssetRegistry::TryGet()};

    mTextureReference.Resolve(Resolver);
    mPipelineReference.Resolve(Resolver);
}

void UBillboardComponent::Serialize(FArchive& Archive) {
    Super::Serialize(Archive);

    if (Archive.IsLoading()) {
        OnRenderStateChanged();
    }
}

bool UBillboardComponent::TryGetBillBoardWorld(FMatrix& OutWorld) const {
    if (!CanRenderBillBoard()) {
        return false;
    }

    OutWorld = GetComponentToWorld();

    return true;
}

void UBillboardComponent::SetTextureHandle(FAssetHandle InTextureHandle) {
    mTextureReference.Assign(InTextureHandle, FAssetRegistry::TryGet());
    OnRenderStateChanged();
}

void UBillboardComponent::SetPipelineHandle(FAssetHandle InPipelineHandle) {
    mPipelineReference.Assign(InPipelineHandle, FAssetRegistry::TryGet());
    OnRenderStateChanged();
}

void UBillboardComponent::SetSize(const FVector2& InSize) {
    if (mSize.mX == InSize.mX && mSize.mY == InSize.mY) {
        return;
    }

    mSize = InSize;
    OnRenderStateChanged();
}

void UBillboardComponent::SetUV(const FVector2& InUVMin, const FVector2& InUVMax) {
    if (mUvMin.mX == InUVMin.mX && mUvMin.mY == InUVMin.mY && mUvMax.mX == InUVMax.mX && mUvMax.mY == InUVMax.mY) {
        return;
    }

    mUvMin = InUVMin;
    mUvMax = InUVMax;
    MarkRenderDynamicDataDirty();
}

void UBillboardComponent::SetColor(const FVector4& InColor) {
    if (mColor == InColor) {
        return;
    }

    mColor = InColor;
    MarkRenderStateDirty();
}

FAssetHandle UBillboardComponent::GetTextureHandle() const {
    return mTextureReference.GetHandle();
}

FAssetHandle UBillboardComponent::GetPipelineHandle() const {
    return mPipelineReference.GetHandle();
}

const FVector2& UBillboardComponent::GetSize() const {
    return mSize;
}

const FVector2& UBillboardComponent::GetUVmin() const {
    return mUvMin;
}

const FVector2& UBillboardComponent::GetUVMax() const {
    return mUvMax;
}

const FVector4& UBillboardComponent::GetColor() const {
    return mColor;
}

bool UBillboardComponent::BuildBillboardDrawData(FBillboardDrawData& OutData) const {
    if (!mTextureReference.GetHandle() || !mPipelineReference.GetHandle()) {
        return false;
    }

    if (!TryGetBillBoardWorld(OutData.mWorld)) {
        return false;
    }

    OutData.mTextureHandle = mTextureReference.GetHandle();
    OutData.mPipelineHandle = mPipelineReference.GetHandle();
    OutData.mSize = mSize;
    OutData.mUvMin = mUvMin;
    OutData.mUvMax = mUvMax;
    OutData.mColor = mColor;

    return true;
}

bool UBillboardComponent::GetWorldCorners(const FMatrix& CameraWorld, std::array<FVector3, 4>& OutCorners) const {
    FBillboardDrawData Data{};

    if (!BuildBillboardDrawData(Data) || Data.mSize.mX <= 0.0f || Data.mSize.mY <= 0.0f) {
        return false;
    }

    FVector3 Right{CameraWorld.m_[0][0], CameraWorld.m_[0][1], CameraWorld.m_[0][2]};
    FVector3 Up{CameraWorld.m_[1][0], CameraWorld.m_[1][1], CameraWorld.m_[1][2]};

    if (Right.LengthSquared() <= 0.0f || Up.LengthSquared() <= 0.0f) {
        return false;
    }

    Right.Normalize();
    Up.Normalize();

    const FVector3 Origin{Data.mWorld.Translation()};
    const FVector3 Horizontal{Right * (Data.mSize.mX * 0.5f)};
    const FVector3 Vertical{Up * (Data.mSize.mY * 0.5f)};

    OutCorners = {Origin - Horizontal + Vertical, Origin - Horizontal - Vertical, Origin + Horizontal + Vertical, Origin + Horizontal - Vertical};

    return true;
}

bool UBillboardComponent::ShouldCreateRenderState() const {
    return IsVisible();
}

std::unique_ptr<FPrimitiveSceneProxy> UBillboardComponent::CreateSceneProxy() const {
    const AActor* Owner{GetOwner()};
    FBillboardDrawData Data{};

    if (!IsRegistered() || !IsVisible() || Owner == nullptr || !BuildBillboardDrawData(Data)) {
        return nullptr;
    }

    FPrimitiveTransform Transform{GetRenderTransform()};

    Transform.mWorld = Data.mWorld;

    std::unique_ptr<FBillboardSceneProxy> Proxy{std::make_unique<FBillboardSceneProxy>(GetHandle(), Owner->GetHandle(), Transform, Data)};

    Proxy->SetUVTransforms(GetUVTransforms());
    return Proxy;
}

void UBillboardComponent::SendRenderTransform() {
    FPrimitiveTransform Transform{GetRenderTransform()};

    if (GetBelongingWorld() != nullptr && IsRenderStateCreated() && TryGetBillBoardWorld(Transform.mWorld)) {
        GetBelongingWorld()->GetRenderSubsystem().UpdatePrimitiveTransform(GetHandle(), Transform);
    }
}

void UBillboardComponent::SendRenderDynamicData() {
    Super::SendRenderDynamicData();
    if (GetBelongingWorld() != nullptr && IsRenderStateCreated()) {
        GetBelongingWorld()->GetRenderSubsystem().UpdateBillboardUV(GetHandle(), mUvMin, mUvMax);
    }
}

UBillboardComponent::UBillboardComponent() {
    InitializeClass(StaticClass());
}

void UBillboardComponent::SerializeTextureHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UBillboardComponent& Object{*static_cast<UBillboardComponent*>(Container)};

    SerializeAssetReference(Archive, "TextureAssetGuid", "TextureAssetPath", Object.mTextureReference);
}

void UBillboardComponent::SerializePipelineHandleProperty(void* Container, FArchive& Archive, const FProperty& Property) {
    UBillboardComponent& Object{*static_cast<UBillboardComponent*>(Container)};

    SerializeAssetReference(Archive, "PipelineAssetGuid", "PipelineAssetPath", Object.mPipelineReference);
}
