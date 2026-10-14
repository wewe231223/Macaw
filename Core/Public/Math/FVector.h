#pragma once

#include <iostream>
#include "Core/Types.h"
#include "Math/SimpleMath/SimpleMath.h"

struct FMatrix;

struct FVector {
    union {
        float V[3];

        struct
        {
            float X;
            float Y;
            float Z;
        };

        struct
        {
            float mX;
            float mY;
            float mZ;
        };
    };

public:
    /* Constructor */
    FVector();
    FVector(float x, float y, float z);
    explicit FVector(float f);
    explicit FVector(const DirectX::XMFLOAT3& Value);
    // ~FVector();

public:
    /* Public Functions */
    void Set(float x, float y, float z);

    float Size() const;   // 길이 반환
    float Length() const; // ==  size()
    float Dot(const FVector& V1) const;

    FVector Cross(const FVector& V1) const;
    FVector GetAbs() const;
    void Normalize();

    float LengthSquared() const;
    DirectX::SimpleMath::Vector3 ToSimpleMath() const;

    // float GetMax();
    //// float GetMin();
    // float GetAbsMax();
    // float GetAbsMin();

    // GetSafeNormal
    // IsNearlyZero
    // Equals
    // ClampSize

    /* operator */

    FVector operator-();
    FVector operator-() const;

    FVector operator-(const FVector& V1) const;
    FVector& operator-=(const FVector& V1);

    FVector operator+(const FVector& V1) const;
    FVector& operator+=(const FVector& V1);

    FVector operator*(const FVector& V1) const;
    FVector operator*(const float& f) const;
    FVector& operator*=(const FVector& V1);
    FVector& operator*=(const float& f);

    FVector operator/(const FVector& V1) const;
    FVector operator/(const float& f) const;
    FVector& operator/=(const FVector& V1);
    FVector& operator/=(const float& f);

    FVector operator^(const FVector& V1) const;

    bool operator==(const FVector& V1) const;
    bool operator!=(const FVector& V1) const;

    float operator[](int32 Index) const;
    float& operator[](int32 Index);

    /* Static */
    static float Dot(const FVector& V1, const FVector& V2);
    static FVector Cross(const FVector& V1, const FVector& V2);
    static float Distance(const FVector& V1, const FVector& V2); // == Dist()

    static const FVector Zero;
    static const FVector UnitX;
    static const FVector UnitY;
    static const FVector UnitZ;
    static const FVector OneVector;

    static FVector Min(const FVector& A, const FVector& B);
    static FVector Max(const FVector& A, const FVector& B);
    static FVector Transform(const FVector& Position, const FMatrix& Matrix);
    static FVector TransformNormal(const FVector& Direction, const FMatrix& Matrix);
};

using FVector3 = FVector;

inline const FVector FVector::Zero{};
inline const FVector FVector::UnitX{1.0f, 0.0f, 0.0f};
inline const FVector FVector::UnitY{0.0f, 1.0f, 0.0f};
inline const FVector FVector::UnitZ{0.0f, 0.0f, 1.0f};

/* Global Operator */
std::ostream& operator<<(std::ostream& OS, const FVector& V);
