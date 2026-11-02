#include "Asset/Sequence/ULevelSequence.h"
#include "Core/Archive.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>

namespace Asset {

    namespace {
        constexpr Core::Uint32 SequenceFormatVersion{1};
    }

    Core::Int32 Asset::ULevelSequence::GetDisplayRate() const {
        return mDisplayRate;
    }

    Core::Int32 Asset::ULevelSequence::GetStartTick() const {
        return mStartTick;
    }

    Core::Int32 Asset::ULevelSequence::GetEndTick() const {
        return mEndTick;
    }

    const Core::TArray<Asset::FSequenceBinding>& Asset::ULevelSequence::GetBindings() const {
        return mBindings;
    }

    Core::TArray<Asset::FSequenceBinding>& Asset::ULevelSequence::GetBindings() {
        return mBindings;
    }

    const Core::TArray<Asset::FSequenceCameraCut>& Asset::ULevelSequence::GetCameraCuts() const {
        return mCameraCuts;
    }

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

    void Asset::ULevelSequence::RemoveCameraCut(Core::Int32 Tick) {
        std::erase_if(mCameraCuts, [Tick](const Asset::FSequenceCameraCut& Cut) {
            return Cut.mTick == Tick;
        });
    }

    void Asset::ULevelSequence::RemoveCameraCuts(const Core::FGuid& CameraBinding) {
        std::erase_if(mCameraCuts, [&CameraBinding](const Asset::FSequenceCameraCut& Cut) {
            return Cut.mCameraBinding == CameraBinding;
        });
    }

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

    const Asset::FSequenceCameraCut* Asset::ULevelSequence::FindCameraCut(double Tick) const {
        const auto Next{std::upper_bound(mCameraCuts.begin(), mCameraCuts.end(), Tick, [](double Value, const Asset::FSequenceCameraCut& Cut) {
            return Value < Cut.mTick;
        })};

        return Next != mCameraCuts.begin() ? &*std::prev(Next) : nullptr;
    }

    bool Asset::ULevelSequence::SetDisplayRate(Core::Int32 DisplayRate) {
        if (DisplayRate <= 0 || TicksPerSecond % DisplayRate != 0) {
            return false;
        }

        mDisplayRate = DisplayRate;

        return true;
    }

    void Asset::ULevelSequence::SetEndTick(Core::Int32 EndTick) {
        mEndTick = EndTick > mStartTick ? EndTick : mStartTick;
    }

    Core::Int32 Asset::ULevelSequence::FrameToTick(Core::Int32 Frame) const {
        return Frame * (TicksPerSecond / mDisplayRate);
    }

    Core::Int32 Asset::ULevelSequence::TickToFrame(double Tick) const {
        return static_cast<Core::Int32>(std::lround(Tick * mDisplayRate / TicksPerSecond));
    }

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

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
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

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
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
