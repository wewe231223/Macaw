#pragma once

#include "Asset/Import/FMeshImportBone.h"
#include "Core/FRevision.h"
#include "CoreUObject/UObject.h"
#include "USkeleton.generated.h"

namespace Asset {
    inline constexpr Core::Uint32 MaxSkeletonBones{100000};

    UCLASS()
    class USkeleton final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        USkeleton() = default;
        ~USkeleton() override = default;

    public:
        bool SetBones(Core::TArray<Asset::FMeshImportBone> Bones);
        const Core::TArray<Asset::FMeshImportBone>& GetBones() const;
        const Core::TArray<Core::FMatrix>& GetGlobalBindPoses() const;
        const Core::TArray<Core::FMatrix>& GetInverseBindPoses() const;

        Core::FRevision GetRevision() const;
        bool IsValid() const;

    private:
        Core::TArray<Asset::FMeshImportBone> mBones{};
        Core::TArray<Core::FMatrix> mGlobalBindPoses{};
        Core::TArray<Core::FMatrix> mInverseBindPoses{};
        Core::FRevision mRevision{};
    };
}
