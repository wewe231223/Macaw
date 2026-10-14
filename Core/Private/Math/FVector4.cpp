#include "pch.h"
#include "Math/FVector4.h"
#include "Math/FVector.h"
#include <assert.h>
#include <cmath>

struct FVector;
struct FMatrix;

FVector4::FVector4() {
    X = 0.0f;
    Y = 0.0f;
    Z = 0.0f;
    W = 0.0f;
}

FVector4::FVector4(float x, float y, float z, float w) {
    X = x;
    Y = y;
    Z = z;
    W = w;
}

FVector4::FVector4(float f) {
    X = f;
    Y = f;
    Z = f;
    W = f;
}

FVector4::FVector4(const FVector& V1, float w) {
    X = V1.X;
    Y = V1.Y;
    Z = V1.Z;
    W = w;
}

/* Functions */

void FVector4::Set(float x, float y, float z, float w) {
    X = x;
    Y = y;
    Z = z;
    W = w;
}

void FVector4::Set(const FVector& V1, float w) {
    X = V1.X;
    Y = V1.Y;
    Z = V1.Z;
    W = w;
}

void FVector4::Set(const FVector4& V1) {
    X = V1.X;
    Y = V1.Y;
    Z = V1.Z;
    W = V1.W;
}

float FVector4::Size() const {
    float sum = X * X + Y * Y + Z * Z + W * W;

    return sqrt(sum);
}

float FVector4::Length() const {
    float sum = X * X + Y * Y + Z * Z + W * W;

    return sqrt(sum);
}

float FVector4::LengthSquared() const {
    return X * X + Y * Y + Z * Z + W * W;
}

void FVector4::Normalize() {
    const float size = Size();

    if (!FMath::IsNearlyZero(size)) {
        X /= size;
        Y /= size;
        Z /= size;
        W /= size;
    }
}

float FVector4::Dot(const FVector4& V1) const {
    return X * V1.X + Y * V1.Y + Z * V1.Z + W * V1.W;
}

FVector4 FVector4::Cross(const FVector4& V1) const {
    return FVector4(Y * V1.Z - Z * V1.Y, Z * V1.X - X * V1.Z, X * V1.Y - Y * V1.X, 0);
}

FVector4 FVector4::GetAbs() const {
    return FVector4{std::abs(X), std::abs(Y), std::abs(Z), std::abs(W)};
}

/* Operator */

FVector4 FVector4::operator-() {
    return FVector4(-X, -Y, -Z, -W);
}

FVector4 FVector4::operator-(const FVector4& V1) const {
    return FVector4(X - V1.X, Y - V1.Y, Z - V1.Z, W - V1.W);
}

FVector4& FVector4::operator-=(const FVector4& V1) {
    X -= V1.X;
    Y -= V1.Y;
    Z -= V1.Z;
    W -= V1.W;

    return *this;
}

FVector4 FVector4::operator+(const FVector4& V1) const {
    return FVector4(X + V1.X, Y + V1.Y, Z + V1.Z, W + V1.W);
}

FVector4& FVector4::operator+=(const FVector4& V1) {
    X += V1.X;
    Y += V1.Y;
    Z += V1.Z;
    W += V1.W;

    return *this;
}

FVector4 FVector4::operator*(const FVector4& V1) const {
    return FVector4(X * V1.X, Y * V1.Y, Z * V1.Z, W * V1.W);
}

FVector4 FVector4::operator*(const float& f) const {
    return FVector4(X * f, Y * f, Z * f, W * f);
}

FVector4 FVector4::operator*(const FMatrix& M) const {
    return M.TransformFVector4(*this);
    /*return FVector4(
		X * M[0][0] + Y * M[1][0] + Z * M[2][0] + W * M[3][0],
		X * M[0][1] + Y * M[1][1] + Z * M[2][1] + W * M[3][1],
		X * M[0][2] + Y * M[1][2] + Z * M[2][2] + W * M[3][2],
		X * M[0][3] + Y * M[1][3] + Z * M[2][3] + W * M[3][3]
	);*/
}

FVector4& FVector4::operator*=(const FVector4& V1) {
    X *= V1.X;
    Y *= V1.Y;
    Z *= V1.Z;
    W *= V1.W;

    return *this;
}

FVector4& FVector4::operator*=(const float& f) {
    X *= f;
    Y *= f;
    Z *= f;
    W *= f;

    return *this;
}

FVector4 FVector4::operator/(const FVector4& V1) const {
    assert(V1.X != 0 && V1.Y != 0 && V1.Z != 0 && V1.W != 0);

    return FVector4(X / V1.X, Y / V1.Y, Z / V1.Z, W / V1.W);
}

FVector4 FVector4::operator/(const float& f) const {
    return FVector4(X / f, Y / f, Z / f, W / f);
}

FVector4& FVector4::operator/=(const FVector4& V1) {
    X /= V1.X;
    Y /= V1.Y;
    Z /= V1.Z;
    W /= V1.W;

    return *this;
}

FVector4& FVector4::operator/=(const float& f) {
    X /= f;
    Y /= f;
    Z /= f;
    W /= f;

    return *this;
}

// 스트림 출력 연산자와 함께 사용할 시 괄호로 묶을 것
FVector4 FVector4::operator^(const FVector4& V1) const {
    return Cross(V1);
}

bool FVector4::operator==(const FVector4& V1) const {
    return (X == V1.X) && (Y == V1.Y) && (Z == V1.Z) && (W == V1.W);
}

bool FVector4::operator!=(const FVector4& V1) const {
    return !(*this == V1);
}

float FVector4::operator[](int32 Index) const {
    return V[Index];
}

float& FVector4::operator[](int32 Index) {
    return V[Index];
}

/* Global Operator */
std::ostream& operator<<(std::ostream& OS, const FVector4& V) {
    OS << "(" << V.X << ", " << V.Y << ", " << V.Z << ", " << V.W << ")";

    return OS;
}

/* Static Functions */

float FVector4::Distance(const FVector4& V1, const FVector4& V2) {
    float dX = V1.X - V2.X;
    float dY = V1.Y - V2.Y;
    float dZ = V1.Z - V2.Z;
    float dW = V1.W - V2.W;
    float sum = dX * dX + dY * dY + dZ * dZ + dW * dW;

    return sqrt(sum);
}
