/**
 * @file UAnimSequence.h
 * @brief 본별 키 트랙을 일정 간격으로 저장한 애니메이션 클립 에셋입니다.
 * @ingroup Asset
 */

#pragma once

#include "Core/Types.h"
#include "Asset/Animation/FAnimDataModel.h"
#include "Asset/Animation/UAnimSequenceBase.h"
#include "Core/Math/FTransform.h"
#include "UAnimSequence.generated.h"

namespace Asset {

    class FAssetManager;

    /**
     * @brief 루트 모션 추출 시 포즈의 루트 본을 고정할 기준입니다.
     * @ingroup Asset
     */
    UENUM()
    enum class ERootMotionRootLock : Core::Uint8 {

        /**
         * @brief 스켈레톤의 기준 포즈에 루트 본을 고정합니다.
         */
        RefPose,

        /**
         * @brief 시퀀스 첫 프레임의 루트 변환에 고정합니다.
         */
        AnimFirstFrame,

        /**
         * @brief 항등 변환에 루트 본을 고정합니다.
         */
        Zero
    };

    /**
     * @brief 임포트한 본 트랙과 프레임 레이트를 소유하는 애니메이션 클립입니다.
     */
    UCLASS()
    class UAnimSequence final : public Asset::UAnimSequenceBase {
        GENERATED_BODY()

    public:

        /**
         * @brief UAnimSequence의 초기 상태와 의존성을 구성합니다.
         */
        UAnimSequence() = default;

        /**
         * @brief 객체의 수명을 끝내고 소유한 리소스를 정리합니다.
         */
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

        /**
         * @brief 로드된 시퀀스의 에셋 경로를 반환합니다.
         * @return 시퀀스가 보관하는 경로 참조입니다.
         */
        const Core::FString& GetAssetPath() const;

        /**
         * @brief 시퀀스의 루트 모션 추출 설정을 확인합니다.
         * @return 추출을 허용하면 true입니다.
         */
        bool IsRootMotionEnabled() const;

        /**
         * @brief 시퀀스의 루트 모션 추출 허용 여부를 설정합니다.
         * @param Enabled 기능 활성화 여부입니다.
         */
        void SetRootMotionEnabled(bool Enabled);

        /**
         * @brief 루트 본 고정 기준을 반환합니다.
         * @return 현재 시퀀스의 루트 고정 방식입니다.
         */
        Asset::ERootMotionRootLock GetRootMotionRootLock() const;

        /**
         * @brief 루트 모션 추출 시 포즈의 루트 고정 기준을 설정합니다.
         * @param RootLock 루트 본을 고정할 기준입니다.
         */
        void SetRootMotionRootLock(Asset::ERootMotionRootLock RootLock);

        /**
         * @brief 루트 모션을 추출하지 않을 때도 루트 고정을 적용하는지 확인합니다.
         * @return 강제 루트 고정이 켜져 있으면 true입니다.
         */
        bool IsRootLockForced() const;

        /**
         * @brief 루트 모션 추출 여부와 별개로 루트 고정을 강제할지 설정합니다.
         * @param Forced 추출하지 않을 때도 루트 본을 고정할지 여부입니다.
         */
        void SetForceRootLock(bool Forced);

        /**
         * @brief 루트 모션 추출 본의 설정 이름을 반환합니다.
         * @return 이름의 읽기 전용 참조이며 빈 값은 기본 루트를 뜻합니다.
         */
        const Core::FString& GetRootMotionBone() const;

        /**
         * @brief 루트 모션을 추출할 본 이름을 설정합니다.
         * @param BoneName 루트 모션 본 이름이며 빈 값은 기본 루트를 사용합니다.
         * @return 본 이름 설정을 허용하면 true입니다.
         */
        bool SetRootMotionBone(const Core::FString& BoneName);

        /**
         * @brief 루트 트랙을 지정 시각에 샘플링합니다.
         * @param TrackIndex 시퀀스 안의 대상 트랙 인덱스입니다.
         * @param Time 시퀀스 또는 콘텐츠 시각(초)입니다.
         * @return 해당 시각의 루트 변환입니다.
         */
        Core::FTransform GetRootTrackTransform(Core::Index_t TrackIndex, double Time) const;

        /**
         * @brief 루트 고정 방식에 맞는 기준 포즈를 계산합니다.
         * @param TrackIndex 시퀀스 안의 대상 트랙 인덱스입니다.
         * @param ReferencePose 기준 포즈 방식에서 사용할 로컬 변환입니다.
         * @return 기준 포즈, 첫 프레임 또는 항등 변환입니다.
         */
        Core::FTransform GetRootLockTransform(Core::Index_t TrackIndex, const Core::FTransform& ReferencePose) const;

        /**
         * @brief 반복하지 않는 두 시각 사이에서 루트 트랙 변환의 차이를 추출합니다.
         * @param TrackIndex 시퀀스 안의 대상 트랙 인덱스입니다.
         * @param StartTime 구간 시작 시각(초)입니다.
         * @param EndTime 구간 종료 시각(초)입니다.
         * @return 구간의 루트 이동·회전 증분입니다.
         */
        Core::FTransform ExtractRootMotionFromRange(Core::Index_t TrackIndex, double StartTime, double EndTime) const;

        /**
         * @brief 반복 경계와 재생 방향을 고려하여 루트 트랙의 이동 증분을 추출합니다.
         * @param TrackIndex 시퀀스 안의 대상 트랙 인덱스입니다.
         * @param StartTime 구간 시작 시각(초)입니다.
         * @param DeltaTime 루트 모션을 추출할 시간 길이(초)이며 음수는 역방향입니다.
         * @param Looping 끝점을 넘어 반복할지 여부입니다.
         * @return 지정 시간 구간의 루트 변환이며 유효한 트랙이 없으면 항등 변환입니다.
         */
        Core::FTransform ExtractRootMotion(Core::Index_t TrackIndex, double StartTime, double DeltaTime, bool Looping) const;


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

        /**
         * @brief 내부 상태를 구성·직렬화하는 FAssetManager에 접근을 허용합니다.
         */
        friend class Asset::FAssetManager;

    private:

        /**
         * @brief 시퀀스 트랙을 시간에 따라 보간하여 샘플링합니다.
         * @param TrackIndex 시퀀스 안의 대상 트랙 인덱스입니다.
         * @param Time 시퀀스 또는 콘텐츠 시각(초)입니다.
         * @return 보간한 트랙의 로컬 변환입니다.
         */
        Core::FTransform SampleTrackTransform(Core::Index_t TrackIndex, double Time) const;

    private:

        /**
         * @brief 시퀀스에서 루트 모션 추출을 허용하는지 여부입니다.
         */
        UPROPERTY(Edit, Name = "EnableRootMotion", Getter = IsRootMotionEnabled, Setter = SetRootMotionEnabled)
        bool mEnableRootMotion{};

        /**
         * @brief 추출 시 루트 본을 고정할 기준입니다.
         */
        UPROPERTY(Edit, Name = "RootMotionRootLock", Getter = GetRootMotionRootLock, Setter = SetRootMotionRootLock)
        Asset::ERootMotionRootLock mRootMotionRootLock{Asset::ERootMotionRootLock::AnimFirstFrame};

        /**
         * @brief 추출하지 않는 경우에도 루트 본 고정을 적용하는지 여부입니다.
         */
        UPROPERTY(Edit, Name = "ForceRootLock", Getter = IsRootLockForced, Setter = SetForceRootLock)
        bool mForceRootLock{};

        /**
         * @brief 루트 모션을 추출할 본 이름이며 빈 값은 기본 루트를 사용합니다.
         */
        UPROPERTY(Edit, Name = "RootMotionBone", Getter = GetRootMotionBone, Setter = SetRootMotionBone)
        Core::FString mRootMotionBone{};

        /**
         * @brief 본 트랙과 시간 정보입니다.
         */
        Asset::FAnimDataModel mDataModel{};

        /**
         * @brief 에셋 관리자에 등록된 시퀀스의 경로입니다.
         */
        Core::FString mAssetPath{};

    };

}
