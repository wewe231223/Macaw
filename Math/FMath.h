#pragma once
#include <algorithm>

#include "SimpleMath/SimpleMath.h"

struct FQuat;
struct FMatrix;

using FPlane = DirectX::SimpleMath::Plane;
using FRay = DirectX::SimpleMath::Ray;

struct FVector2 {
    float mX{0.0f};
    float mY{0.0f};

    constexpr FVector2() = default;

    FVector2(float InX, float InY);

    float LengthSquared() const;

    float Length() const;

    void Normalize();

    FVector2 operator+(const FVector2& Other) const;

    FVector2 operator-(const FVector2& Other) const;

    FVector2 operator*(float Scalar) const;

    FVector2& operator/=(float Scalar);

    FVector2& operator+=(const FVector2& Other);
};

struct FVector {
    float mX{0.0f};
    float mY{0.0f};
    float mZ{0.0f};

    constexpr FVector() = default;

    FVector(float InX, float InY, float InZ);

    explicit FVector(const DirectX::XMFLOAT3& Value);

    DirectX::SimpleMath::Vector3 ToSimpleMath() const;

    static FVector Transform(const FVector& Position, const FMatrix& Matrix);
    static FVector TransformNormal(const FVector& Direction, const FMatrix& Matrix);
    static const FVector Zero, UnitX, UnitY, UnitZ;
    bool operator==(const FVector&) const = default;

    FVector operator-() const;

    static FVector Min(const FVector& A, const FVector& B);

    static FVector Max(const FVector& A, const FVector& B);

    FVector operator+(const FVector& Rhs) const;

    FVector operator-(const FVector& Rhs) const;

    FVector operator*(float Scalar) const;

    FVector operator/(float Scalar) const;

    float Dot(const FVector& Rhs) const;

    FVector Cross(const FVector& Rhs) const;

    float LengthSquared() const;

    float Length() const;

    void Normalize();
};

inline const FVector FVector::Zero{};
inline const FVector FVector::UnitX{1, 0, 0};
inline const FVector FVector::UnitY{0, 1, 0};
inline const FVector FVector::UnitZ{0, 0, 1};

struct FVector4 {
    float mX{0.0f};
    float mY{0.0f};
    float mZ{0.0f};
    float mW{0.0f};

    constexpr FVector4() = default;

    FVector4(float InX, float InY, float InZ, float InW);

    FVector4(FVector InXYZ, float InW);

    float Dot(const FVector4& Rhs) const;

    float LengthSquared() const;

    float Length() const;

    //float Length3Squared() const;
    //float Length3() const;

    FVector4 operator+(const FVector4& Rhs) const;

    FVector4 operator-(const FVector4& Rhs) const;

    FVector4 operator*(float Scalar) const;

    FVector4& operator+=(const FVector4& Rhs);

    FVector4& operator-=(const FVector4& Rhs);

    FVector4& operator*=(float Scalar);

    FVector4& operator/=(float Scalar);
};

using FVector3 = FVector;
using FVector2D = FVector2;
using FColor4 = FVector4;

struct FRotator {
    // Z-up convention: pitch rotates around X, yaw around Z, and roll around Y.
    float mX{0.0f}; // pitch
    float mY{0.0f}; // yaw
    float mZ{0.0f}; // roll

    constexpr FRotator() = default;

    FRotator(float InPitch, float InYaw, float InRoll);

    FRotator(const FVector& InEuler);

    operator FVector() const;

    constexpr bool operator==(const FRotator&) const = default;

    bool operator==(const FVector& Other) const;

    static const FRotator Zero;
};

inline const FRotator FRotator::Zero{};

struct FMatrix {
    FMatrix Invert() const;

    static const FMatrix Identity;

    static FMatrix CreateScale(float X, float Y, float Z);

    static FMatrix CreateTranslation(float X, float Y, float Z);

    float m_[4][4]{ {1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}};

    FMatrix operator*(const FMatrix& Rhs) const;

    static FMatrix CreateScale(const FVector& Scale);

    static FMatrix CreateTranslation(const FVector& Position);

    static FMatrix CreateRotationX(float Radians);

    static FMatrix CreateRotationY(float Radians);

    static FMatrix CreateRotationZ(float Radians);

    static FMatrix CreateFromYawPitchRoll(float Yaw, float Pitch, float Roll);

    FVector TransformPosition(const FVector& Position) const;

    FVector TransformDirection(const FVector& Direction) const;

    FVector Translation() const;

    void Translation(const FVector& Position);

    FVector Right() const;

    FVector Up() const;

    FVector Forward() const;

    static FMatrix CreatePerspectiveFieldOfView(float FovY, float AspectRatio, float NearZ, float FarZ);

    bool TransformCoord(const FVector& Position, FVector& OutPosition) const;

    bool TryInverse(FMatrix& OutInverse) const;

    static FMatrix CreateOrthographic(float Width, float Height, float NearZ, float FarZ);

    static FMatrix CreateFromQuaternion(const FQuat& Rotation);

    DirectX::SimpleMath::Matrix ToSimpleMath() const;

    FQuat ToQuaternion() const;

    bool Decompose(FVector& OutScale, FQuat& OutRotation, FVector& OutTranslation) const;
};

struct FQuat {
    float mX{};
    float mY{};
    float mZ{};
    float mW{};

    FQuat();

    FQuat(float X, float Y, float Z, float W);

    FQuat(const FVector& V, float W);

    FQuat(const FVector4& V);

    explicit FQuat(const DirectX::XMFLOAT4& Value);

    DirectX::SimpleMath::Quaternion ToSimpleMath() const;

    FQuat operator*(const FQuat& Other) const;

    static FQuat FromRotator(const FRotator& Rotation);

    FRotator ToRotator() const;

    void Normalize();
    ;

    FQuat Inverse() const;

    static FQuat Concatenate(const FQuat& First, const FQuat& Second);

    const static FQuat CreateFromRotationMatrix(const FMatrix& M);

    const FVector ToEuler() const;

    const static FQuat CreateFromAxisAngle(const FVector Axis, const float Angle);
};

inline const FMatrix FMatrix::Identity{};
