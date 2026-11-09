#pragma once

#include "Asset/Import/FMeshImportBone.h"
#include "Core/Math/FTransform.h"
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

        /**
         * @brief 본별 바인드 포즈를 위치·회전·스케일로 분해한 레퍼런스 포즈를 반환합니다.
         * @return 본 인덱스 순서의 부모 본 기준 변환이며, 애니메이션 트랙이 없는 본은 이 값을 씁니다.
         */
        const Core::TArray<Core::FTransform>& GetReferencePose() const;

        bool IsValid() const;

    private:
        Core::TArray<Asset::FMeshImportBone> mBones{};
        Core::TArray<Core::FMatrix> mGlobalBindPoses{};
        Core::TArray<Core::FMatrix> mInverseBindPoses{};
        Core::TArray<Core::FTransform> mReferencePose{};
    };
}
