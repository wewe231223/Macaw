#include "../doctest/doctest.h"

#include <concepts>
#include <numbers>

#include "../FMath.h"

static_assert(std::same_as<FVector2D, DirectX::SimpleMath::Vector2>);
static_assert(std::same_as<FVector3, DirectX::SimpleMath::Vector3>);
static_assert(std::same_as<FVector4, DirectX::SimpleMath::Vector4>);
static_assert(std::same_as<FRotator, DirectX::SimpleMath::Vector3>);
static_assert(std::same_as<FQuat, DirectX::SimpleMath::Quaternion>);
static_assert(std::same_as<FMatrix, DirectX::SimpleMath::Matrix>);
static_assert(std::same_as<FPlane, DirectX::SimpleMath::Plane>);
static_assert(std::same_as<FRay, DirectX::SimpleMath::Ray>);

TEST_SUITE("Math aliases") {
    TEST_CASE("FVector2D supports arithmetic and length calculations") {
        const FVector2D First{ 3.0f, 4.0f };
        const FVector2D Second{ 1.0f, 2.0f };

        const FVector2D Sum = First + Second;
        const FVector2D Difference = First - Second;

        CHECK_EQ(Sum.x, 4.0f);
        CHECK_EQ(Sum.y, 6.0f);
        CHECK_EQ(Difference.x, 2.0f);
        CHECK_EQ(Difference.y, 2.0f);
        CHECK(First.Length() == doctest::Approx(5.0f));
        CHECK(First.Dot(Second) == doctest::Approx(11.0f));
    }

    TEST_CASE("FVector3 supports dot, cross, and normalization") {
        const FVector3 XAxis{ 1.0f, 0.0f, 0.0f };
        const FVector3 YAxis{ 0.0f, 1.0f, 0.0f };
        const FVector3 ZAxis = XAxis.Cross(YAxis);

        CHECK(ZAxis.x == doctest::Approx(0.0f));
        CHECK(ZAxis.y == doctest::Approx(0.0f));
        CHECK(ZAxis.z == doctest::Approx(1.0f));
        CHECK(XAxis.Dot(YAxis) == doctest::Approx(0.0f));

        FVector3 Direction{ 2.0f, 0.0f, 0.0f };
        Direction.Normalize();

        CHECK(Direction.x == doctest::Approx(1.0f));
        CHECK(Direction.y == doctest::Approx(0.0f));
        CHECK(Direction.z == doctest::Approx(0.0f));
        CHECK(Direction.Length() == doctest::Approx(1.0f));
    }

    TEST_CASE("FVector4 preserves all four components") {
        const FVector4 Value{ 1.0f, 2.0f, 3.0f, 4.0f };

        CHECK_EQ(Value.x, 1.0f);
        CHECK_EQ(Value.y, 2.0f);
        CHECK_EQ(Value.z, 3.0f);
        CHECK_EQ(Value.w, 4.0f);
        CHECK(Value.LengthSquared() == doctest::Approx(30.0f));
    }

    TEST_CASE("FRotator can create a normalized quaternion") {
        const FRotator Rotation{ 0.0f, std::numbers::pi_v<float> / 2.0f, 0.0f };
        const FQuat Quaternion = FQuat::CreateFromYawPitchRoll(Rotation);

        CHECK(Quaternion.Length() == doctest::Approx(1.0f));
        CHECK(Quaternion.Dot(FQuat::Identity) < 1.0f);
    }

    TEST_CASE("FMatrix transforms a position with translation") {
        const FMatrix Translation = FMatrix::CreateTranslation(10.0f, -2.0f, 5.0f);
        const FVector3 Position{ 1.0f, 2.0f, 3.0f };

        const FVector3 Transformed = FVector3::Transform(Position, Translation);

        CHECK(Transformed.x == doctest::Approx(11.0f));
        CHECK(Transformed.y == doctest::Approx(0.0f));
        CHECK(Transformed.z == doctest::Approx(8.0f));
        CHECK(Translation.Translation() == FVector3{ 10.0f, -2.0f, 5.0f });
    }

    TEST_CASE("FPlane reports signed point distances") {
        const FPlane Ground{ FVector3::UnitY, 0.0f };

        CHECK(Ground.DotCoordinate(FVector3{ 0.0f, 3.0f, 0.0f }) == doctest::Approx(3.0f));
        CHECK(Ground.DotCoordinate(FVector3{ 0.0f, -2.0f, 0.0f }) == doctest::Approx(-2.0f));
        CHECK(Ground.DotCoordinate(FVector3::Zero) == doctest::Approx(0.0f));
    }

    TEST_CASE("FRay intersects a plane at the expected distance") {
        const FRay Ray{ FVector3{ 0.0f, 5.0f, 0.0f }, FVector3{ 0.0f, -1.0f, 0.0f } };
        const FPlane Ground{ FVector3::UnitY, 0.0f };
        float Distance = 0.0f;

        REQUIRE(Ray.Intersects(Ground, Distance));
        CHECK(Distance == doctest::Approx(5.0f));

        const FVector3 Intersection = Ray.position + Ray.direction * Distance;
        CHECK(Intersection.x == doctest::Approx(0.0f));
        CHECK(Intersection.y == doctest::Approx(0.0f));
        CHECK(Intersection.z == doctest::Approx(0.0f));
    }
}
