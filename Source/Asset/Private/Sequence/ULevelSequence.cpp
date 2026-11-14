/**
 * @file ULevelSequence.cpp
 * @brief 시네마틱 재생 구간과 바인딩을 저장하는 시퀀스 에셋에 대한 선언입니다.
 * @ingroup Asset
 */

#include "Core/Types.h"
#include "Asset/Sequence/ULevelSequence.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>

namespace Asset {

    /**
     * @brief 이 파일에서 사용하는 내부 보조 기능과 상태를 묶습니다.
     */
    namespace {
        /**
         * @brief 레벨 시퀀스의 저장 형식 버전입니다.
         */
        constexpr Core::Uint32 SequenceFormatVersion{1};
    }

    /**
     * @brief 편집 화면에 표시할 초당 프레임 수를 반환합니다.
     */
    Core::Int32 Asset::ULevelSequence::GetDisplayRate() const {
        return mDisplayRate;
    }

    /**
     * @brief 재생 시작 시간을 반환합니다.
     */
    Core::Int32 Asset::ULevelSequence::GetStartTick() const {
        return mStartTick;
    }

    /**
     * @brief 재생 끝 시간을 반환합니다.
     */
    Core::Int32 Asset::ULevelSequence::GetEndTick() const {
        return mEndTick;
    }

    /**
     * @brief 바인딩 목록을 반환합니다.
     */
    const Core::TArray<Asset::FSequenceBinding>& Asset::ULevelSequence::GetBindings() const {
        return mBindings;
    }

    /**
     * @brief 편집할 바인딩 목록을 반환합니다.
     */
    Core::TArray<Asset::FSequenceBinding>& Asset::ULevelSequence::GetBindings() {
        return mBindings;
    }

    /**
     * @brief 시간순으로 정렬된 카메라 컷 목록을 반환합니다.
     */
    const Core::TArray<Asset::FSequenceCameraCut>& Asset::ULevelSequence::GetCameraCuts() const {
        return mCameraCuts;
    }

    /**
     * @brief 카메라 컷을 시간순 위치에 추가하고 같은 시간의 컷은 교체합니다.
     */
    void Asset::ULevelSequence::AddCameraCut(Core::Int32 Tick, const Core::FGuid& CameraBinding) {
        const auto Position{std::lower_bound(mCameraCuts.begin(), mCameraCuts.end(), Tick, [](const Asset::FSequenceCameraCut& Cut, Core::Int32 Value) {
            return Cut.mTick < Value;
        })};

        if (Position != mCameraCuts.end() && Position->mTick == Tick) {
            Position->mCameraBinding = CameraBinding;

            return;
        }

        Asset::FSequenceCameraCut Cut{};
        Cut.mTick = Tick;
        Cut.mCameraBinding = CameraBinding;

        mCameraCuts.insert(Position, Cut);
    }

    /**
     * @brief 지정한 시간의 카메라 컷을 제거합니다.
     */
    void Asset::ULevelSequence::RemoveCameraCut(Core::Int32 Tick) {
        std::erase_if(mCameraCuts, [Tick](const Asset::FSequenceCameraCut& Cut) {
            return Cut.mTick == Tick;
        });
    }

    /**
     * @brief 지정한 바인딩을 가리키는 카메라 컷을 모두 제거합니다.
     */
    void Asset::ULevelSequence::RemoveCameraCuts(const Core::FGuid& CameraBinding) {
        std::erase_if(mCameraCuts, [&CameraBinding](const Asset::FSequenceCameraCut& Cut) {
            return Cut.mCameraBinding == CameraBinding;
        });
    }

    /**
     * @brief 카메라 컷을 다른 시간으로 옮깁니다.
     */
    bool Asset::ULevelSequence::MoveCameraCut(Core::Int32 FromTick, Core::Int32 ToTick) {
        const auto HasTick{[](Core::Int32 Tick) {
            return [Tick](const Asset::FSequenceCameraCut& Cut) {
                return Cut.mTick == Tick;
            };
        }};

        const auto Source{std::find_if(mCameraCuts.begin(), mCameraCuts.end(), HasTick(FromTick))};

        if (Source == mCameraCuts.end() || std::any_of(mCameraCuts.begin(), mCameraCuts.end(), HasTick(ToTick))) {
            return false;
        }

        const Core::FGuid CameraBinding{Source->mCameraBinding};

        mCameraCuts.erase(Source);
        AddCameraCut(ToTick, CameraBinding);

        return true;
    }

    /**
     * @brief 지정한 시간에 적용할 카메라 컷을 찾습니다.
     */
    const Asset::FSequenceCameraCut* Asset::ULevelSequence::FindCameraCut(double Tick) const {
        const auto Next{std::upper_bound(mCameraCuts.begin(), mCameraCuts.end(), Tick, [](double Value, const Asset::FSequenceCameraCut& Cut) {
            return Value < Cut.mTick;
        })};

        return Next != mCameraCuts.begin() ? &*std::prev(Next) : nullptr;
    }

    /**
     * @brief 표시 프레임 수를 설정합니다.
     */
    bool Asset::ULevelSequence::SetDisplayRate(Core::Int32 DisplayRate) {
        if (DisplayRate <= 0 || TicksPerSecond % DisplayRate != 0) {
            return false;
        }

        mDisplayRate = DisplayRate;

        return true;
    }

    /**
     * @brief 재생 끝 시간을 설정하고 시작 시간 이상으로 맞춥니다.
     */
    void Asset::ULevelSequence::SetEndTick(Core::Int32 EndTick) {
        mEndTick = EndTick > mStartTick ? EndTick : mStartTick;
    }

    /**
     * @brief 표시 프레임을 틱으로 변환합니다.
     */
    Core::Int32 Asset::ULevelSequence::FrameToTick(Core::Int32 Frame) const {
        return Frame * (TicksPerSecond / mDisplayRate);
    }

    /**
     * @brief 틱을 가장 가까운 표시 프레임으로 변환합니다.
     */
    Core::Int32 Asset::ULevelSequence::TickToFrame(double Tick) const {
        return static_cast<Core::Int32>(std::lround(Tick * mDisplayRate / TicksPerSecond));
    }

    /**
     * @brief 아카이브 모드에 따라 값을 읽거나 저장합니다.
     */
    void Asset::ULevelSequence::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Version{SequenceFormatVersion};

        Archive.Serialize("FormatVersion", Version);
        Archive.Serialize("DisplayRate", mDisplayRate);
        Archive.Serialize("StartTick", mStartTick);
        Archive.Serialize("EndTick", mEndTick);

        if (Archive.HasError()) {
            return;
        }

        if (Version != SequenceFormatVersion || mDisplayRate <= 0 || mEndTick < mStartTick) {
            Archive.SetError("Invalid sequence version, display rate or range");

            return;
        }

        Core::Uint32 Count{static_cast<Core::Uint32>(mBindings.size())};

        if (!Archive.BeginArray("Bindings", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mBindings.clear();
        }

        for (Core::Index_t Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                mBindings.emplace_back();
            }

            mBindings[Index].Serialize(Archive);
            Archive.EndObject();
        }

        Archive.EndArray();

        if (Archive.HasError() || (Archive.IsLoading() && !Archive.HasField("CameraCuts"))) {
            return;
        }

        Count = static_cast<Core::Uint32>(mCameraCuts.size());

        if (!Archive.BeginArray("CameraCuts", Count)) {
            return;
        }

        Core::TArray<Asset::FSequenceCameraCut> Saved{mCameraCuts};

        if (Archive.IsLoading()) {
            Saved.assign(Count, Asset::FSequenceCameraCut{});
            mCameraCuts.clear();
        }

        for (Core::Index_t Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            Archive.Serialize("Tick", Saved[Index].mTick);
            Archive.Serialize("Camera", Saved[Index].mCameraBinding);

            if (Archive.IsLoading() && !Archive.HasError()) {
                AddCameraCut(Saved[Index].mTick, Saved[Index].mCameraBinding);
            }

            Archive.EndObject();
        }

        Archive.EndArray();
    }

}
