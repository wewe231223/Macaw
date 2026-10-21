#include "Core/Math/FMatrix.h"
#include "Core/Math/FVector.h"
#include "Core/Math/FRotator.h"
#include "Core/Math.h"

#include <DirectXMath.h>
#include <bit>
#include <cmath>

using namespace DirectX;

namespace {
    XMMATRIX LoadMatrix(const FMatrix& Value) {
        const XMFLOAT4X4 Storage{std::bit_cast<XMFLOAT4X4>(Value)};
        return XMLoadFloat4x4(&Storage);
    }

    FMatrix StoreMatrix(FXMMATRIX Value) {
        XMFLOAT4X4 Storage{};
        XMStoreFloat4x4(&Storage, Value);

        return std::bit_cast<FMatrix>(Storage);
    }
}

FMatrix::FMatrix() {
    for (Int32 Index{}; Index < 4; ++Index) {
        mValues[Index][Index] = 1.0f;
    }
}

FMatrix FMatrix::Identity() {
    return {};
}

FMatrix FMatrix::Rotation(const FRotator& Value) {
    return StoreMatrix(XMMatrixRotationX(Math::ToRadians(Value.mRoll)) * XMMatrixRotationY(-Math::ToRadians(Value.mPitch)) * XMMatrixRotationZ(Math::ToRadians(Value.mYaw)));
}

FMatrix FMatrix::RotationAxis(const FVector& Axis, float Radians) {
    return StoreMatrix(XMMatrixRotationAxis(XMVectorSet(Axis.mX, Axis.mY, Axis.mZ, 0.0f), Radians));
}

FMatrix FMatrix::LookTo(const FVector& Position, const FVector& Forward, const FVector& Up) {
    return StoreMatrix(XMMatrixLookToLH(XMVectorSet(Position.mX, Position.mY, Position.mZ, 1.0f), XMVectorSet(Forward.mX, Forward.mY, Forward.mZ, 0.0f), XMVectorSet(Up.mX, Up.mY, Up.mZ, 0.0f)));
}

FMatrix FMatrix::Perspective(float FieldOfView, float Aspect, float NearPlane, float FarPlane) {
    return StoreMatrix(XMMatrixPerspectiveFovLH(Math::ToRadians(FieldOfView), Aspect, NearPlane, FarPlane));
}

FMatrix FMatrix::operator*(const FMatrix& Other) const {
    return StoreMatrix(LoadMatrix(*this) * LoadMatrix(Other));
}

FVector FMatrix::TransformPosition(const FVector& Position) const {
    XMFLOAT3 Result{};
    XMStoreFloat3(&Result, XMVector3TransformCoord(XMVectorSet(Position.mX, Position.mY, Position.mZ, 1.0f), LoadMatrix(*this)));

    return {Result.x, Result.y, Result.z};
}

FVector FMatrix::TransformVector(const FVector& Vector) const {
    XMFLOAT3 Result{};
    XMStoreFloat3(&Result, XMVector3TransformNormal(XMVectorSet(Vector.mX, Vector.mY, Vector.mZ, 0.0f), LoadMatrix(*this)));

    return {Result.x, Result.y, Result.z};
}

FVector FMatrix::GetAxis(Int32 Index) const {
    return {mValues[Index][0], mValues[Index][1], mValues[Index][2]};
}

FRotator FMatrix::GetRotation() const {
    const FVector Forward{GetAxis(0).Normalized()};
    const float Horizontal{std::sqrt(Forward.mX * Forward.mX + Forward.mY * Forward.mY)};

    const float Pitch{std::atan2(Forward.mZ, Horizontal)};
    const float Yaw{Horizontal > 0.000001f ? std::atan2(Forward.mY, Forward.mX) : std::atan2(-GetAxis(1).mX, GetAxis(1).mY)};
    const float Roll{Horizontal > 0.000001f ? std::atan2(GetAxis(1).mZ, GetAxis(2).mZ) : 0.0f};

    return {Math::ToDegrees(Pitch), Math::ToDegrees(Yaw), Math::ToDegrees(Roll)};
}

bool FMatrix::TryInverse(FMatrix& Result) const {
    XMVECTOR Determinant{};
    const XMMATRIX Inverse{XMMatrixInverse(&Determinant, LoadMatrix(*this))};
    const float Value{XMVectorGetX(Determinant)};

    if (!std::isfinite(Value) || std::abs(Value) < 0.00000001f) {
        return false;
    }

    Result = StoreMatrix(Inverse);

    return true;
}
