#include "pch.h"
#include "World/Component/FMeshPickingProxy.h"
#include "Asset/UMesh.h"

const UMesh* FMeshPickingProxy::GetMesh() const {
    return mSource != nullptr ? mSource->Mesh : nullptr;
}

void FMeshPickingProxy::Update(const UMesh* InMesh, const FTransform& Transform) {
    mSource = InMesh != nullptr ? InMesh->GetPickingSource() : nullptr;

    const FVector3& Scale{Transform.GetScale()};

    mValid = std::isfinite(Scale.mX) && std::isfinite(Scale.mY) && std::isfinite(Scale.mZ) && Scale.mX != 0.0f && Scale.mY != 0.0f && Scale.mZ != 0.0f;

    if (!mValid)
        return;

    mReverseWinding = (Scale.mX < 0.0f) ^ (Scale.mY < 0.0f) ^ (Scale.mZ < 0.0f);
    mInverseScale = {1.0f / Scale.mX, 1.0f / Scale.mY, 1.0f / Scale.mZ};
    DirectX::XMStoreFloat3(&mPosition, Transform.GetPosition().ToSimpleMath());
    DirectX::XMStoreFloat4(&mRotation, DirectX::XMQuaternionNormalize(Transform.GetRotationQuaternion().ToSimpleMath()));
}

bool FMeshPickingProxy::PrepareRay(const FRay& Ray, FRay& LocalRay, double& DirectionLength) const {
    if (!mValid)
        return false;

    const auto Scale{DirectX::XMLoadFloat3(&mInverseScale)};
    const auto Q{DirectX::XMLoadFloat4(&mRotation)};
    const auto Origin{DirectX::XMVectorMultiply(DirectX::XMVector3InverseRotate(DirectX::XMVectorSubtract(Ray.position, DirectX::XMLoadFloat3(&mPosition)), Q), Scale)};
    const auto Direction{DirectX::XMVectorMultiply(DirectX::XMVector3InverseRotate(Ray.direction, Q), Scale)};
    const double DX{DirectX::XMVectorGetX(Direction)};
    const double DY{DirectX::XMVectorGetY(Direction)};
    const double DZ{DirectX::XMVectorGetZ(Direction)};

    DirectionLength = std::sqrt(DX * DX + DY * DY + DZ * DZ);

    if (!std::isfinite(DirectionLength) || DirectionLength <= 0.0 || DirectX::XMVector3IsNaN(Origin) || DirectX::XMVector3IsInfinite(Origin))
        return false;

    LocalRay = FRay{Origin, DirectX::XMVectorSet(static_cast<float>(DX / DirectionLength), static_cast<float>(DY / DirectionLength), static_cast<float>(DZ / DirectionLength), 0.0f)};

    return true;
}

bool FMeshPickingProxy::RaycastPrepared(const FRay& LocalRay, double DirectionLength, FWorldRaycastHit& OutHit, float MaxDistance) const {
    if (mSource == nullptr || !(MaxDistance >= 0.0f))
        return false;

    const double Limit{static_cast<double>(MaxDistance) * DirectionLength};
    const float LocalLimit{Limit >= std::numeric_limits<float>::max() ? std::numeric_limits<float>::max() : std::bit_cast<float>(std::bit_cast<Uint32>((std::max)(0.0f, static_cast<float>(Limit))) + 1u)};
    FMeshRaycastHit Hit{};

    if (!mSource->Raycast(LocalRay, Hit, LocalLimit, mReverseWinding))
        return false;

    const float WorldDistance{static_cast<float>(Hit.mDistance / DirectionLength)};

    if (WorldDistance > MaxDistance)
        return false;

    OutHit.mDistance = WorldDistance;
    OutHit.mNormal = TransformNormal(Hit.mNormal, mReverseWinding);
    OutHit.mFaceIndex = Hit.mFaceIndex;

    return true;
}

bool FMeshPickingProxy::Raycast(const FRay& Ray, FWorldRaycastHit& OutHit, float MaxDistance) const {
    FRay LocalRay{};
    double DirectionLength{};

    FRay WorldRay{};
    const FWorldRaycastQuery Query{MaxDistance};

    if (!Query.PrepareRay(Ray, WorldRay) || !PrepareRay(WorldRay, LocalRay, DirectionLength) || !RaycastPrepared(LocalRay, DirectionLength, OutHit, MaxDistance)) {
        return false;
    }

    OutHit.mPosition = FVector3{WorldRay.position} + FVector3{WorldRay.direction} * OutHit.mDistance;
    return true;
}

FVector3 FMeshPickingProxy::TransformNormal(const FVector3& Normal, bool Reverse) const {
    const double X{static_cast<double>(Normal.mX) * mInverseScale.x};
    const double Y{static_cast<double>(Normal.mY) * mInverseScale.y};
    const double Z{static_cast<double>(Normal.mZ) * mInverseScale.z};
    const double Length{std::sqrt(X * X + Y * Y + Z * Z)};

    if (!std::isfinite(Length) || Length <= 0.0) {
        return {};
    }

    const double Sign{Reverse ? -1.0 : 1.0};
    const auto Scaled{DirectX::XMVectorSet(static_cast<float>(Sign * X / Length), static_cast<float>(Sign * Y / Length), static_cast<float>(Sign * Z / Length), 0.0f)};
    DirectX::XMFLOAT3 Result{};

    DirectX::XMStoreFloat3(&Result, DirectX::XMVector3Rotate(Scaled, DirectX::XMLoadFloat4(&mRotation)));
    return FVector3{Result};
}
