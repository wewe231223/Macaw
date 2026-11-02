#include "Asset/Sequence/FSequenceTransformTrack.h"
#include "Core/Archive.h"

#include <string>

namespace Asset {

    Asset::ESequenceTrackType Asset::FSequenceTransformTrack::GetType() const {
        return Asset::ESequenceTrackType::Transform;
    }

    void Asset::FSequenceTransformTrack::Serialize(Core::FArchive& Archive) {
        Core::Uint32 Count{static_cast<Core::Uint32>(mSections.size())};

        if (!Archive.BeginArray("Sections", Count)) {
            return;
        }

        if (Archive.IsLoading()) {
            mSections.clear();
        }

        for (Core::Uint32 Index{}; Index < Count && !Archive.HasError(); ++Index) {
            if (!Archive.BeginObject(std::to_string(Index))) {
                break;
            }

            if (Archive.IsLoading()) {
                mSections.emplace_back();
            }

            mSections[Index].Serialize(Archive);
            Archive.EndObject();
        }

        Archive.EndArray();
    }

    Asset::FSequenceTransformSection& Asset::FSequenceTransformTrack::FindOrAddEditSection() {
        if (mSections.empty()) {
            mSections.emplace_back();
        }

        return mSections.front();
    }

    void Asset::FSequenceTransformTrack::ExpandSections(Core::Int32 Tick) {
        for (Asset::FSequenceTransformSection& Section : mSections) {
            Section.ExpandToInclude(Tick);
        }
    }

    const Asset::FSequenceTransformSection* Asset::FSequenceTransformTrack::FindSection(double Tick) const {
        for (auto Section{mSections.rbegin()}; Section != mSections.rend(); ++Section) {
            if (Section->Contains(Tick)) {
                return &*Section;
            }
        }

        return nullptr;
    }

}
