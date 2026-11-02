#include "Asset/Sequence/FSequencePropertyTrack.h"
#include "Core/Archive.h"

#include <utility>

namespace Asset {

    Asset::ESequenceTrackType Asset::FSequencePropertyTrack::GetType() const {
        return Asset::ESequenceTrackType::Property;
    }

    void Asset::FSequencePropertyTrack::Serialize(Core::FArchive& Archive) {
        Archive.Serialize("Property", mPropertyName);

        if (Archive.HasError()) {
            return;
        }

        if (mPropertyName.empty()) {
            Archive.SetError("Sequence property track has no property name");
            return;
        }

        if (Archive.BeginObject("Channel")) {
            mChannel.Serialize(Archive);
            Archive.EndObject();
        }
    }

    const Core::FString& Asset::FSequencePropertyTrack::GetPropertyName() const {
        return mPropertyName;
    }

    void Asset::FSequencePropertyTrack::SetPropertyName(Core::FString PropertyName) {
        mPropertyName = std::move(PropertyName);
    }

    Asset::FSequenceChannel& Asset::FSequencePropertyTrack::GetChannel() {
        return mChannel;
    }

    const Asset::FSequenceChannel& Asset::FSequencePropertyTrack::GetChannel() const {
        return mChannel;
    }

}
