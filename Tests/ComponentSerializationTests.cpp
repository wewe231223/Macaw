#include "PCH.h"
#include "../doctest/doctest.h"

#include "../Serialize/FArchiveMemory.h"
#include "../Scene/Component/UCameraComponent.h"
#include "../Scene/Component/UStaticMeshComponent.h"

TEST_SUITE("Component Serialization Tests")
{
    TEST_CASE("Static mesh component restores its inherited and local state")
    {
        UStaticMeshComponent Component;
        Component.SetActive(false);
        Component.SetVisible(false);
        Component.GetTransform().SetPosition({ 1.0f, 2.0f, 3.0f });
        Component.GetTransform().SetRotation({ 0.1f, 0.2f, 0.3f });
        Component.GetTransform().SetScale({ 2.0f, 3.0f, 4.0f });
        Component.SetMeshHandle({ 1, 2 });
        Component.SetMaterialHandle({ 3, 4 });
        Component.SetPipelineHandle({ 5, 6 });

        std::vector<uint8> Bytes;
        FArchiveMemory SaveArchive(Bytes);
        Component.Save(SaveArchive);

        Component.SetActive(true);
        Component.SetVisible(true);
        Component.GetTransform().SetPosition({});
        Component.GetTransform().SetRotation({});
        Component.GetTransform().SetScale({ 1.0f, 1.0f, 1.0f });
        Component.SetMeshHandle({});
        Component.SetMaterialHandle({});
        Component.SetPipelineHandle({});

        const std::vector<uint8> SavedBytes = Bytes;
        FArchiveMemory LoadArchive(SavedBytes);
        Component.Load(LoadArchive);

        CHECK_FALSE(Component.IsActive());
        CHECK_FALSE(Component.IsVisible());
        CHECK(Component.GetTransform().GetPosition().x == doctest::Approx(1.0f));
        CHECK(Component.GetTransform().GetPosition().y == doctest::Approx(2.0f));
        CHECK(Component.GetTransform().GetPosition().z == doctest::Approx(3.0f));
        CHECK(Component.GetTransform().GetRotation().x == doctest::Approx(0.1f));
        CHECK(Component.GetTransform().GetRotation().y == doctest::Approx(0.2f));
        CHECK(Component.GetTransform().GetRotation().z == doctest::Approx(0.3f));
        CHECK(Component.GetTransform().GetScale().x == doctest::Approx(2.0f));
        CHECK(Component.GetTransform().GetScale().y == doctest::Approx(3.0f));
        CHECK(Component.GetTransform().GetScale().z == doctest::Approx(4.0f));
        CHECK(Component.GetMeshHandle() == FAssetHandle{ 1, 2 });
        CHECK(Component.GetMaterialHandle() == FAssetHandle{ 3, 4 });
        CHECK(Component.GetPipelineHandle() == FAssetHandle{ 5, 6 });
    }

    TEST_CASE("Camera component restores its inherited and local state")
    {
        UCameraComponent Component;
        Component.SetActive(false);
        Component.GetTransform().SetPosition({ 7.0f, 8.0f, 9.0f });
        Component.SetFOV(0.75f);
        Component.SetAspectRatio(2.0f);
        Component.SetNearPlane(0.25f);
        Component.SetFarPlane(500.0f);

        std::vector<uint8> Bytes;
        FArchiveMemory SaveArchive(Bytes);
        Component.Save(SaveArchive);

        Component.SetActive(true);
        Component.GetTransform().SetPosition({});
        Component.SetFOV(1.0f);
        Component.SetAspectRatio(1.0f);
        Component.SetNearPlane(1.0f);
        Component.SetFarPlane(1.0f);

        const std::vector<uint8> SavedBytes = Bytes;
        FArchiveMemory LoadArchive(SavedBytes);
        Component.Load(LoadArchive);

        CHECK_FALSE(Component.IsActive());
        CHECK(Component.GetTransform().GetPosition().x == doctest::Approx(7.0f));
        CHECK(Component.GetTransform().GetPosition().y == doctest::Approx(8.0f));
        CHECK(Component.GetTransform().GetPosition().z == doctest::Approx(9.0f));
        CHECK(Component.GetFOV() == doctest::Approx(0.75f));
        CHECK(Component.GetAspectRatio() == doctest::Approx(2.0f));
        CHECK(Component.GetNearPlane() == doctest::Approx(0.25f));
        CHECK(Component.GetFarPlane() == doctest::Approx(500.0f));
    }
}
