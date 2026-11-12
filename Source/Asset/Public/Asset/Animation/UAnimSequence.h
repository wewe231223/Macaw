/**
 * @file UAnimSequence.h
 * @brief 본별 키 트랙을 일정 간격으로 저장한 애니메이션 클립 에셋입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Animation/FAnimDataModel.h"
#include "Asset/Animation/UAnimSequenceBase.h"
#include "Core/Math/FTransform.h"
#include "UAnimSequence.generated.h"

namespace Asset {

    class FAssetManager;

    UENUM()
    enum class ERootMotionRootLock : Core::Uint8 {
        RefPose,
        AnimFirstFrame,
        Zero
    };

    /**
     * @brief 임포트한 본 트랙과 프레임 레이트를 소유하는 애니메이션 클립입니다.
     */
    UCLASS()
    class UAnimSequence final : public Asset::UAnimSequenceBase {
        GENERATED_BODY()

    public:
        UAnimSequence() = default;

        ~UAnimSequence() override = default;

    public:
        /**
         * @brief 데이터를 검증하고 교체합니다.
         * @param[in] DataModel 새 트랙과 시간 정보입니다.
         * @param[out] Error 검증 실패 이유입니다.
         * @return 유효해 교체했으면 true이며, 실패하면 기존 데이터를 유지합니다.
         */
        bool SetDataModel(Asset::FAnimDataModel DataModel, Core::FString& Error);

        /**
         * @brief 트랙과 시간 정보를 반환합니다.
         * @return 읽기 전용 데이터입니다.
         */
        const Asset::FAnimDataModel& GetDataModel() const;
        const Core::FString& GetAssetPath() const;

        bool IsRootMotionEnabled() const;
        void SetRootMotionEnabled(bool Enabled);

        Asset::ERootMotionRootLock GetRootMotionRootLock() const;
        void SetRootMotionRootLock(Asset::ERootMotionRootLock RootLock);

        bool IsRootLockForced() const;
        void SetForceRootLock(bool Forced);

        const Core::FString& GetRootMotionBone() const;
        bool SetRootMotionBone(const Core::FString& BoneName);

        Core::FTransform GetRootTrackTransform(Core::Int32 TrackIndex, double Time) const;
        Core::FTransform GetRootLockTransform(Core::Int32 TrackIndex, const Core::FTransform& ReferencePose) const;
        Core::FTransform ExtractRootMotionFromRange(Core::Int32 TrackIndex, double StartTime, double EndTime) const;
        Core::FTransform ExtractRootMotion(Core::Int32 TrackIndex, double StartTime, double DeltaTime, bool Looping) const;


        /**
         * @brief 지정한 시간의 트랙별 로컬 변환을 샘플링합니다.
         * @details 앞뒤 키 사이를 위치·스케일은 선형 보간, 회전은 구면 선형 보간합니다. 키가 하나인 채널은 그 값을 그대로 씁니다.
         * 반복 재생이면 시간을 재생 길이로 나눈 나머지를 쓰고, 아니면 0과 재생 길이 사이로 제한합니다.
         * @param[in] Time 초 단위 재생 시간입니다.
         * @param[in] Looping 반복 재생 여부입니다.
         * @param[out] OutTrackPose 트랙 순서의 로컬 변환이며 트랙 수만큼 크기를 맞춥니다.
         */
        void GetTrackPose(double Time, bool Looping, Core::TArray<Core::FTransform>& OutTrackPose) const;

        double GetPlayLength() const override;

        bool IsValid() const override;

    private:
        friend class Asset::FAssetManager;

    private:
        Core::FTransform SampleTrackTransform(std::size_t TrackIndex, double Time) const;

    private:
        UPROPERTY(Edit, Name = "EnableRootMotion", Getter = IsRootMotionEnabled, Setter = SetRootMotionEnabled)
        bool mEnableRootMotion{};

        UPROPERTY(Edit, Name = "RootMotionRootLock", Getter = GetRootMotionRootLock, Setter = SetRootMotionRootLock)
        Asset::ERootMotionRootLock mRootMotionRootLock{Asset::ERootMotionRootLock::AnimFirstFrame};

        UPROPERTY(Edit, Name = "ForceRootLock", Getter = IsRootLockForced, Setter = SetForceRootLock)
        bool mForceRootLock{};

        UPROPERTY(Edit, Name = "RootMotionBone", Getter = GetRootMotionBone, Setter = SetRootMotionBone)
        Core::FString mRootMotionBone{};

        /**
         * @brief 본 트랙과 시간 정보입니다.
         */
        Asset::FAnimDataModel mDataModel{};

        Core::FString mAssetPath{};

    };

}
