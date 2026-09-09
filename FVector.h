#pragma once

#include <cmath>
#include "SimpleMath/SimpleMath.h"

struct FMatrix;

struct FVector
{
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;

	constexpr FVector() = default;

	constexpr FVector(float InX, float InY, float InZ) : x(InX), y(InY), z(InZ) {}

    explicit FVector(const DirectX::XMFLOAT3& value) : x(value.x), y(value.y), z(value.z) {}
    DirectX::SimpleMath::Vector3 ToSimpleMath() const { return {x, y, z}; }

    static FVector Transform(const FVector& position, const FMatrix& matrix);
    static FVector TransformNormal(const FVector& direction, const FMatrix& matrix);
	static const FVector Zero, UnitX, UnitY, UnitZ;
	bool operator==(const FVector&) const = default;
	FVector operator-() const { return FVector(-x, -y, -z); }
	static FVector Min(const FVector& a, const FVector& b)
	{
		return { a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z };
	}
	static FVector Max(const FVector& a, const FVector& b)
	{
		return { a.x > b.x ? a.x : b.x, a.y > b.y ? a.y : b.y, a.z > b.z ? a.z : b.z };
	}

	FVector operator+(const FVector& rhs) const
	{
		return FVector(x + rhs.x, y + rhs.y, z + rhs.z);
	}

	FVector operator-(const FVector& rhs) const
	{
		return FVector(x - rhs.x, y - rhs.y, z - rhs.z);
	}

	FVector operator*(float scalar) const
	{
		return FVector(x * scalar, y * scalar, z * scalar);
	}

	FVector operator/(float scalar) const
	{
		return FVector(x / scalar, y / scalar, z / scalar);
	}

	float Dot(const FVector& rhs) const {
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}

	FVector Cross(const FVector& rhs) const {
		return FVector(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
	}

	float LengthSquared() const
	{
		return Dot(*this);
	}

	float Length() const
	{
		return std::sqrt(LengthSquared());
	}

	void Normalize()
	{
		const float length = Length();

		if (length <= 1e-6f)
		{
			x = y = z = 0.0f;
			return;
		}

		x /= length;
		y /= length;
		z /= length;
	}
};

inline const FVector FVector::Zero{};
inline const FVector FVector::UnitX{1, 0, 0};
inline const FVector FVector::UnitY{0, 1, 0};
inline const FVector FVector::UnitZ{0, 0, 1};
