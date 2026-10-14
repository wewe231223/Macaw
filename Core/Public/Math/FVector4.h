#pragma once
#include "Core/Types.h"

//template<typename T>
//struct TVector;
struct FVector;
struct FMatrix;

struct FVector4 {
    union {
        float V[4];

        struct
        {
            float X;
            float Y;
            float Z;
            float W;
        };

        struct
        {
            float mX;
            float mY;
            float mZ;
            float mW;
        };
    };

    /* Constructor */
    FVector4();
    FVector4(float x, float y, float z, float w);
    explicit FVector4(float f);
    FVector4(const FVector& V1, float w = 0.0f);
    // ~FVector();

public:
    /* Public Functions */
    void Set(float x, float y, float z, float w);
    void Set(const FVector& V1, float w = 0.0f);
    void Set(const FVector4& V1);

    float Size() const;   // 길이 반환
    float Length() const; // ==  size()
    float LengthSquared() const;
    float Dot(const FVector4& V1) const;

    void Normalize();
    FVector4 Cross(const FVector4& V1) const;
    FVector4 GetAbs() const;

    // float GetMax();
    //// float GetMin();
    // float GetAbsMax();
    // float GetAbsMin();

    // GetSafeNormal

    // ClampSize

    /* operator */

    FVector4 operator-();

    FVector4 operator-(const FVector4& V1) const;
    FVector4& operator-=(const FVector4& V1);

    FVector4 operator+(const FVector4& V1) const;
    FVector4& operator+=(const FVector4& V1);

    FVector4 operator*(const FVector4& V1) const;
    FVector4 operator*(const float& f) const;
    FVector4 operator*(const FMatrix& M) const;

    FVector4& operator*=(const FVector4& V1);
    FVector4& operator*=(const float& f);

    FVector4 operator/(const FVector4& V1) const;
    FVector4 operator/(const float& f) const;
    FVector4& operator/=(const FVector4& V1);
    FVector4& operator/=(const float& f);

    FVector4 operator^(const FVector4& V1) const;

    bool operator==(const FVector4& V1) const;
    bool operator!=(const FVector4& V1) const;

    float operator[](int32 Index) const;
    float& operator[](int32 Index);

    static float Distance(const FVector4& V1, const FVector4& V2); // == Dist()
};

using FColor4 = FVector4;

/* Global Operator */
std::ostream& operator<<(std::ostream& OS, const FVector4& V);
