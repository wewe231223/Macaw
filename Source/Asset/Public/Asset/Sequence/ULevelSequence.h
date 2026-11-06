/**
 * @file ULevelSequence.h
 * @brief 시네마틱 재생 구간과 바인딩을 저장하는 시퀀스 에셋에 대한 선언입니다.
 * @ingroup Asset
 */

#pragma once

#include "Asset/Sequence/FSequenceBinding.h"
#include "Asset/Sequence/FSequenceCameraCut.h"
#include "CoreUObject/UObject.h"
#include "ULevelSequence.generated.h"

namespace Asset {

    /**
     * @brief 시네마틱 재생 구간과 바인딩을 저장하는 시퀀스 에셋입니다.
     * @ingroup Asset
     */
    UCLASS()
    class ULevelSequence final : public CoreUObject::UObject {
        GENERATED_BODY()

    public:
        /**
         * @brief 키와 재생 구간이 사용하는 초당 틱 수입니다.
         */
        static constexpr Core::Int32 TicksPerSecond{24000};

    public:
        /**
         * @brief 편집 화면에 표시할 초당 프레임 수를 반환합니다.
         * @return 양수인 초당 프레임 수입니다.
         */
        Core::Int32 GetDisplayRate() const;

        /**
         * @brief 재생 시작 시간을 반환합니다.
         * @return 틱 단위 시작 시간입니다.
         */
        Core::Int32 GetStartTick() const;

        /**
         * @brief 재생 끝 시간을 반환합니다.
         * @return 틱 단위 끝 시간입니다.
         */
        Core::Int32 GetEndTick() const;

        /**
         * @brief 바인딩 목록을 반환합니다.
         * @return 시퀀스가 소유한 바인딩 목록의 참조입니다.
         */
        const Core::TArray<Asset::FSequenceBinding>& GetBindings() const;

        /**
         * @brief 편집할 바인딩 목록을 반환합니다.
         * @return 시퀀스가 소유한 바인딩 목록의 참조입니다.
         */
        Core::TArray<Asset::FSequenceBinding>& GetBindings();

        /**
         * @brief 시간순으로 정렬된 카메라 컷 목록을 반환합니다.
         * @return 시퀀스가 소유한 카메라 컷 목록의 참조입니다.
         */
        const Core::TArray<Asset::FSequenceCameraCut>& GetCameraCuts() const;

        /**
         * @brief 카메라 컷을 시간순 위치에 추가하고 같은 시간의 컷은 교체합니다.
         * @param[in] Tick 컷이 시작하는 시간이며 틱 단위입니다.
         * @param[in] CameraBinding 카메라를 가리키는 바인딩 ID입니다.
         */
        void AddCameraCut(Core::Int32 Tick, const Core::FGuid& CameraBinding);

        /**
         * @brief 지정한 시간의 카메라 컷을 제거합니다.
         * @param[in] Tick 제거할 컷의 시간이며 틱 단위입니다.
         */
        void RemoveCameraCut(Core::Int32 Tick);

        /**
         * @brief 지정한 바인딩을 가리키는 카메라 컷을 모두 제거합니다.
         * @param[in] CameraBinding 카메라를 가리키는 바인딩 ID입니다.
         */
        void RemoveCameraCuts(const Core::FGuid& CameraBinding);

        /**
         * @brief 카메라 컷을 다른 시간으로 옮깁니다.
         * @param[in] FromTick 옮길 컷의 시간이며 틱 단위입니다.
         * @param[in] ToTick 새 시간이며 틱 단위입니다.
         * @return 원본 컷이 있고 새 시간에 다른 컷이 없어 옮기면 true입니다.
         */
        bool MoveCameraCut(Core::Int32 FromTick, Core::Int32 ToTick);

        /**
         * @brief 지정한 시간에 적용할 카메라 컷을 찾습니다.
         * @param[in] Tick 조회할 시간이며 틱 단위입니다.
         * @return 이 시간 이전에 시작한 마지막 컷이며 없으면 nullptr입니다.
         */
        const Asset::FSequenceCameraCut* FindCameraCut(double Tick) const;

        /**
         * @brief 표시 프레임 수를 설정합니다.
         * @param[in] DisplayRate TicksPerSecond를 나누어떨어지게 하는 양수입니다.
         * @return 값을 적용하면 true입니다.
         */
        bool SetDisplayRate(Core::Int32 DisplayRate);

        /**
         * @brief 재생 끝 시간을 설정하고 시작 시간 이상으로 맞춥니다.
         * @param[in] EndTick 틱 단위 끝 시간입니다.
         */
        void SetEndTick(Core::Int32 EndTick);

        /**
         * @brief 표시 프레임을 틱으로 변환합니다.
         * @param[in] Frame 표시 프레임 번호입니다.
         * @return 틱 단위 시간입니다.
         */
        Core::Int32 FrameToTick(Core::Int32 Frame) const;

        /**
         * @brief 틱을 가장 가까운 표시 프레임으로 변환합니다.
         * @param[in] Tick 틱 단위 시간입니다.
         * @return 표시 프레임 번호입니다.
         */
        Core::Int32 TickToFrame(double Tick) const;

        void Serialize(Core::FArchive& Archive) override;

    private:
        /**
         * @brief 편집 화면에 표시할 초당 프레임 수입니다.
         */
        Core::Int32 mDisplayRate{30};

        /**
         * @brief 틱 단위 재생 시작 시간입니다.
         */
        Core::Int32 mStartTick{};

        /**
         * @brief 틱 단위 재생 끝 시간입니다.
         */
        Core::Int32 mEndTick{5 * TicksPerSecond};

        /**
         * @brief 시퀀스가 소유한 바인딩 목록입니다.
         */
        Core::TArray<Asset::FSequenceBinding> mBindings{};

        /**
         * @brief 시간순으로 정렬된 카메라 컷 목록입니다.
         */
        Core::TArray<Asset::FSequenceCameraCut> mCameraCuts{};
    };

}
