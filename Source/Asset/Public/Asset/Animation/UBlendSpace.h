/**
 * @file UBlendSpace.h
 * @brief 입력 값에 따라 여러 애니메이션 시퀀스를 섞는 블렌드 스페이스 에셋의 기반입니다.
 * @ingroup Asset
 */

#pragma once

#include "CoreUObject/TObjectRef.h"

#include "Core/Types.h"
#include "Asset/Animation/UAnimationAsset.h"
#include "Core/Math/FVector.h"
#include "UBlendSpace.generated.h"

namespace Asset {

    class FAssetManager;
    class UAnimSequence;

    /**
     * @brief 블렌드 공간 파라미터를 지정하는 축과 축 개수입니다.
     * @ingroup Asset
     */
    enum class EBlendSpaceAxis : Core::Uint8 {

        /**
         * @brief 첫 번째 블렌드 파라미터 축입니다.
         */
        X,

        /**
         * @brief 두 번째 블렌드 파라미터 축입니다.
         */
        Y,

        /**
         * @brief 세 번째 블렌드 파라미터 축입니다.
         */
        Z,

        /**
         * @brief 축 개수이며 파라미터 축으로 사용하지 않습니다.
         */
        Max
    };

    /**
     * @brief 블렌드 스페이스 축 하나의 이름과 범위입니다.
     */
    struct FBlendParameter {
    public:

        /**
         * @brief 에디터에 표시하는 축 이름입니다. 예를 들어 Speed입니다.
         */
        Core::FString mDisplayName{"Speed"};

        /**
         * @brief 축의 최솟값입니다. 입력은 이 값으로 제한됩니다.
         */
        float mMin{0.0f};

        /**
         * @brief 축의 최댓값입니다. 입력은 이 값으로 제한됩니다.
         */
        float mMax{5.0f};

        /**
         * @brief 에디터 격자 칸 수입니다. 샘플을 옮길 때 (최댓값 - 최솟값) / 칸 수 간격에 맞춥니다. 1 이상 100 이하입니다.
         */
        Core::Int32 mGridDivisions{4};
    };

    /**
     * @brief 블렌드 스페이스에 배치한 애니메이션 하나와 그 위치입니다.
     * @details 언리얼 FBlendSample에 대응합니다. 1D 블렌드 스페이스는 mSampleValue의 X만 씁니다.
     */
    struct FBlendSample {
    public:

        /**
         * @brief 경로를 해석한 시퀀스입니다. 에셋 관리자가 소유하며 저장하지 않습니다.
         */
        CoreUObject::TObjectRef<const Asset::UAnimSequence> mAnimation{};

        /**
         * @brief 블렌드 스페이스 좌표계에서 샘플의 위치입니다.
         */
        Core::FVector mSampleValue{};
    };

    /**
     * @brief 입력 값에서 계산한 샘플 하나의 가중치입니다.
     */
    struct FBlendSampleWeight {
    public:

        /**
         * @brief UBlendSpace::GetSamples 안의 샘플 순서입니다.
         */
        Core::Index_t mSampleIndex{-1};

        /**
         * @brief 0보다 크고 1 이하인 가중치이며, 한 번의 계산 결과는 합이 1입니다.
         */
        float mWeight{};
    };

    /**
     * @brief 입력 좌표에 따라 샘플 애니메이션의 가중치를 계산하는 에셋의 기반입니다.
     * @details 저장 파일의 시퀀스 경로는 로드할 때 애니메이션 포인터로 해석합니다.
     */
    UCLASS(Abstract)
    class UBlendSpace : public Asset::UAnimationAsset {
        GENERATED_BODY()

    public:

        /**
         * @brief 한 블렌드 스페이스가 가질 수 있는 최대 샘플 수입니다.
         */
        static constexpr Core::Uint32 MaxSamples{64};

    public:

        /**
         * @brief UBlendSpace의 초기 상태와 의존성을 구성합니다.
         */
        UBlendSpace() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
        ~UBlendSpace() override = default;

    public:

        /**
         * @brief 입력 좌표에서 섞을 샘플과 가중치를 계산합니다.
         * @param[in] Input 블렌드 스페이스 좌표입니다. 축 범위로 제한해 씁니다.
         * @param[out] OutWeights 가중치가 0보다 큰 샘플이며, 시퀀스가 해석된 샘플만 포함합니다. 섞을 샘플이 없으면 비웁니다.
         */
        virtual void GetSamplesToBlend(const Core::FVector& Input, Core::TArray<Asset::FBlendSampleWeight>& OutWeights) const = 0;

        /**
         * @brief 블렌드 스페이스가 쓰는 축 수를 반환합니다.
         * @return 1D이면 1입니다.
         */
        virtual Core::Uint32 GetNumAxes() const = 0;

        /**
         * @brief 축 설정을 반환합니다.
         * @param[in] Axis 설정을 읽을 축입니다.
         * @return 축 설정입니다.
         */
        const Asset::FBlendParameter& GetBlendParameter(Asset::EBlendSpaceAxis Axis) const;

        /**
         * @brief 축 설정을 바꿉니다.
         * @param[in] Axis 블렌드 스페이스가 사용하는 축입니다.
         * @param[in] Parameter 새 설정입니다. 최솟값이 최댓값보다 작은 유한한 값이어야 합니다.
         * @return 적용했으면 true입니다.
         */
        bool SetBlendParameter(Asset::EBlendSpaceAxis Axis, const Asset::FBlendParameter& Parameter);

        /**
         * @brief 샘플을 반환합니다.
         * @return 추가한 순서의 샘플 목록입니다.
         */
        const Core::TArray<Asset::FBlendSample>& GetSamples() const;

        /**
         * @brief 샘플을 추가합니다.
         * @param[in] Animation 샘플에 연결할 시퀀스입니다.
         * @param[in] SampleValue 샘플 위치입니다.
         * @return 추가한 샘플의 순서이며, 샘플 수가 한도에 도달했거나 위치가 유한하지 않으면 -1입니다.
         */
        Core::Index_t AddSample(const Asset::UAnimSequence* Animation, const Core::FVector& SampleValue);

        /**
         * @brief 샘플 하나를 제거합니다.
         * @param[in] Index 제거할 샘플 순서입니다.
         * @return 제거했으면 true입니다.
         */
        bool RemoveSample(Core::Index_t Index);

        /**
         * @brief 샘플의 시퀀스를 바꿉니다.
         * @param[in] Index 샘플 순서입니다.
         * @param[in] Animation 샘플에 연결할 시퀀스입니다.
         * @return 적용했으면 true입니다.
         */
        bool SetSampleAnimation(Core::Index_t Index, const Asset::UAnimSequence* Animation);

        /**
         * @brief 샘플의 위치를 바꿉니다.
         * @param[in] Index 샘플 순서입니다.
         * @param[in] SampleValue 새 위치입니다.
         * @return 위치가 유한해 적용했으면 true입니다.
         */
        bool SetSampleValue(Core::Index_t Index, const Core::FVector& SampleValue);

        /**
         * @brief 가중치를 적용한 샘플 재생 길이의 합을 계산합니다.
         * @details 블렌드 스페이스 플레이어는 이 길이로 정규화 재생 위치를 진행해, 길이가 다른 샘플의 걸음 주기를 맞춥니다.
         * @param[in] Weights GetSamplesToBlend의 결과입니다.
         * @return 초 단위 길이이며 섞을 샘플이 없으면 0입니다.
         */
        double GetBlendedPlayLength(const Core::TArray<Asset::FBlendSampleWeight>& Weights) const;

        /**
         * @brief 해석된 샘플 중 가장 긴 재생 길이를 반환합니다.
         * @return 초 단위 길이입니다.
         */
        double GetPlayLength() const override;

        /**
         * @brief 시퀀스가 해석된 샘플이 하나 이상 있는지 검사합니다.
         * @return 재생할 수 있으면 true입니다.
         */
        bool IsValid() const override;

        /**
         * @brief 블렌드 축과 샘플 목록을 복사하고 파라미터 순서 캐시를 다시 만듭니다.
         * @param Source 복사하거나 읽을 원본 데이터입니다.
         */
        void CopyFrom(const Asset::UBlendSpace& Source);

        /**
         * @brief 축 설정과 샘플의 경로·위치를 읽거나 저장합니다. 해석한 시퀀스는 저장하지 않습니다.
         * @param[in,out] Archive 저장하거나 읽을 아카이브입니다.
         * @param Assets 샘플 시퀀스 참조를 해석할 에셋 관리자입니다.
         */
        void Serialize(Core::FArchive& Archive, Asset::FAssetManager& Assets);

    protected:

        /**
         * @brief 입력 좌표를 축 범위로 제한합니다.
         * @param[in] Input 블렌드 스페이스 좌표입니다.
         * @return 각 축을 [최솟값, 최댓값]으로 제한한 좌표이며, 쓰지 않는 축은 0입니다.
         */
        Core::FVector ClampInput(const Core::FVector& Input) const;

    private:

        /**
         * @brief 파라미터 순으로 샘플 조회 캐시를 재구성합니다.
         */
        virtual void RebuildSampleOrder() = 0;

    private:

        /**
         * @brief 축별 설정입니다. 파생 클래스는 GetNumAxes만큼만 씁니다.
         */
        Asset::FBlendParameter mBlendParameters[static_cast<Core::Uint8>(Asset::EBlendSpaceAxis::Max)]{};

        /**
         * @brief 추가한 순서의 샘플입니다.
         */
        Core::TArray<Asset::FBlendSample> mSamples{};
    };

}
