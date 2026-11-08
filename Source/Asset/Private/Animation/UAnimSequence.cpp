#include "Asset/Animation/UAnimSequence.h"

#include <utility>

namespace Asset {

    bool UAnimSequence::SetDataModel(Asset::FAnimDataModel DataModel, Core::FString& Error) {
        if (!DataModel.Validate(Error)) {
            return false;
        }

        mDataModel = std::move(DataModel);
        mRevision = Core::FRevision::Next();

        return true;
    }

    const Asset::FAnimDataModel& UAnimSequence::GetDataModel() const {
        return mDataModel;
    }

    double UAnimSequence::GetPlayLength() const {
        return mDataModel.GetPlayLength();
    }

    bool UAnimSequence::IsValid() const {
        return !mDataModel.GetBoneAnimationTracks().empty();
    }

    Core::FRevision UAnimSequence::GetRevision() const {
        return mRevision;
    }

}
